#include "bootinfo.h"
#include "gui/font_renderer.h"
#include "gui/gui.h"
#include "string16.h"
#include "uefi.h"

typedef enum { PROC_RUNNING, PROC_EXITED } ProcStatus;
typedef struct Process {
  UINTN pid;
  UINTN ppid;
  CHAR16 cmd[64];
  ProcStatus status;
  struct Process *parent;
  struct Process *first_child;
  struct Process *next_sibling;
} Process;

#define MAX_PROCS 256
#define SPLASH_STALL 2000000
Process proc_table[MAX_PROCS];
UINTN proc_count = 0;
Process *current_proc = NULL;
UINTN next_pid = 1;

int gui_active = 0;

Process *create_process(Process *parent, CHAR16 *cmd);
void execute_command(CHAR16 *cmd);

void print(CHAR16 *str) {
  if (!str)
    return;
  if (gui_active) {
    char buf[256];
    int i = 0;
    while (str[i] && i < 255) {
      buf[i] = (char)str[i];
      i++;
    }
    buf[i] = 0;
    gui_term_print(buf);
  } else {
    ST->ConOut->OutputString(ST->ConOut, str);
  }
}

void print_char(CHAR16 c) {
  if (gui_active) {
    char buf[2];
    buf[0] = (char)c;
    buf[1] = 0;
    gui_term_print(buf);
  } else {
    CHAR16 str[2];
    str[0] = c;
    str[1] = 0;
    ST->ConOut->OutputString(ST->ConOut, str);
  }
}
void print_uint(UINTN n) {
  if (n == 0) {
    print_char('0');
    return;
  }
  CHAR16 buf[32];
  int i = 0;
  while (n > 0) {
    buf[i++] = (n % 10) + '0';
    n /= 10;
  }
  while (i > 0)
    print_char(buf[--i]);
}

Process *create_process(Process *parent, CHAR16 *cmd) {
  if (proc_count >= MAX_PROCS)
    return NULL;
  Process *p = &proc_table[proc_count++];
  p->pid = next_pid++;
  p->ppid = parent ? parent->pid : 0;
  p->status = PROC_RUNNING;
  p->parent = parent;
  p->first_child = NULL;
  p->next_sibling = NULL;
  UINTN i = 0;
  while (cmd[i] && i < 63) {
    p->cmd[i] = cmd[i];
    i++;
  }
  p->cmd[i] = 0;
  if (parent) {
    if (parent->first_child == NULL)
      parent->first_child = p;
    else {
      Process *sibling = parent->first_child;
      while (sibling->next_sibling)
        sibling = sibling->next_sibling;
      sibling->next_sibling = p;
    }
  }
  return p;
}

void print_pstree(Process *p, UINTN indent) {
  if (!p)
    return;
  for (UINTN i = 0; i < indent; i++)
    print((CHAR16 *)L"  ");
  if (indent > 0)
    print((CHAR16 *)L"|- ");
  print(p->cmd);
  print((CHAR16 *)L" [");
  print_uint(p->pid);
  print((CHAR16 *)L"]\r\n");

  Process *child = p->first_child;
  while (child) {
    print_pstree(child, indent + 1);
    child = child->next_sibling;
  }
}

EFI_FILE_PROTOCOL *cwd = NULL;
void get_root(EFI_FILE_PROTOCOL **root) {
  EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
  EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

  ST->BootServices->HandleProtocol(ImgHandle, &loaded_image_guid,
                                   (void **)&LoadedImage);
  ST->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &fs_guid,
                                   (void **)&FileSystem);
  FileSystem->OpenVolume(FileSystem, root);
}

char tgo_buffer[4096];
void tgo_redraw(CHAR16 *filename, UINTN buffer_size, UINTN cursor_pos) {
  ST->ConOut->ClearScreen(ST->ConOut);
  ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
  print((CHAR16 *)L"TGO - Editing: ");
  print(filename);
  print((CHAR16 *)L" | [ESC:Save] "
                  L"[Arrows:Move]\r\n--------------------------------\r\n");

  UINTN cur_x = 0, cur_y = 2, target_x = 0, target_y = 2;
  for (UINTN i = 0; i <= buffer_size; i++) {
    if (i == cursor_pos) {
      target_x = cur_x;
      target_y = cur_y;
    }
    if (i < buffer_size) {
      if (tgo_buffer[i] == '\n') {
        print((CHAR16 *)L"\r\n");
        cur_x = 0;
        cur_y++;
      } else {
        print_char(tgo_buffer[i]);
        cur_x++;
      }
    }
  }
  ST->ConOut->SetCursorPosition(ST->ConOut, target_x, target_y);
}

void cmd_tgo(CHAR16 *filename) {
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  EFI_FILE_PROTOCOL *File;

  if (dir->Open(dir, &File, filename,
                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                0) != EFI_SUCCESS) {
    print((CHAR16 *)L"TGO: Error opening file\r\n");
    return;
  }

  UINTN buffer_size = 4095;
  File->Read(File, &buffer_size, tgo_buffer);
  tgo_buffer[buffer_size] = 0;
  UINTN cursor_pos = buffer_size;

  ST->ConOut->EnableCursor(ST->ConOut, TRUE);
  tgo_redraw(filename, buffer_size, cursor_pos);

  while (1) {
    UINTN idx;
    EFI_INPUT_KEY key;
    ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &idx);

    if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key) != EFI_SUCCESS)
      continue;
    if (key.ScanCode == SCAN_ESC)
      break;

    if (key.ScanCode == SCAN_LEFT && cursor_pos > 0) {
      cursor_pos--;
    } else if (key.ScanCode == SCAN_RIGHT && cursor_pos < buffer_size) {
      cursor_pos++;
    } else if (key.ScanCode == SCAN_UP) {
      UINTN line_start = cursor_pos;
      while (line_start > 0 && tgo_buffer[line_start - 1] != '\n')
        line_start--;
      if (line_start > 0) {
        UINTN col = cursor_pos - line_start;
        UINTN prev_end = line_start - 1;
        UINTN prev_start = prev_end;
        while (prev_start > 0 && tgo_buffer[prev_start - 1] != '\n')
          prev_start--;
        UINTN prev_len = prev_end - prev_start;
        cursor_pos = prev_start + (col < prev_len ? col : prev_len);
      }
    } else if (key.ScanCode == SCAN_DOWN) {
      UINTN line_end = cursor_pos;
      while (line_end < buffer_size && tgo_buffer[line_end] != '\n')
        line_end++;
      if (line_end < buffer_size) {
        UINTN line_start = cursor_pos;
        while (line_start > 0 && tgo_buffer[line_start - 1] != '\n')
          line_start--;
        UINTN col = cursor_pos - line_start;
        UINTN next_start = line_end + 1;
        UINTN next_end = next_start;
        while (next_end < buffer_size && tgo_buffer[next_end] != '\n')
          next_end++;
        UINTN next_len = next_end - next_start;
        cursor_pos = next_start + (col < next_len ? col : next_len);
      }
    } else if (key.UnicodeChar == '\b' && cursor_pos > 0) {
      for (UINTN i = cursor_pos - 1; i < buffer_size; i++)
        tgo_buffer[i] = tgo_buffer[i + 1];
      buffer_size--;
      cursor_pos--;
    } else if (key.UnicodeChar != 0) {
      if (buffer_size < 4094) {
        char c = (key.UnicodeChar == '\r') ? '\n' : (char)key.UnicodeChar;
        for (UINTN i = buffer_size; i > cursor_pos; i--)
          tgo_buffer[i] = tgo_buffer[i - 1];
        tgo_buffer[cursor_pos++] = c;
        buffer_size++;
      }
    }
    tgo_redraw(filename, buffer_size, cursor_pos);
  }

  File->SetPosition(File, 0);
  File->Write(File, &buffer_size, tgo_buffer);
  File->Close(File);
  if (dir != cwd)
    dir->Close(dir);

  ST->ConOut->EnableCursor(ST->ConOut, FALSE);
  ST->ConOut->ClearScreen(ST->ConOut);
}

int tgc_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void tgc_strncpy(char *d, const char *s, int n) {
  int i = 0;
  while (*s && i < n - 1) {
    *d++ = *s++;
    i++;
  }
  *d = 0;
}

int tgc_is_digit(char c) { return c >= '0' && c <= '9'; }

int tgc_is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

typedef enum {
  T_INT,
  T_ID,
  T_NUM,
  T_STRING,
  T_IF,
  T_WHILE,
  T_PRINT,
  T_RETURN,
  T_PLUS,
  T_MINUS,
  T_MUL,
  T_DIV,
  T_LT,
  T_GT,
  T_EQ,
  T_ASSIGN,
  T_LPAREN,
  T_RPAREN,
  T_LBRACE,
  T_RBRACE,
  T_SEMI,
  T_EOF
} TokenType;

typedef struct {
  TokenType type;
  char str[256];
  int val;
} Token;

Token tokens[2048];
int token_cnt = 0;
int t_pos = 0;

struct {
  char name[32];
  int val;
} vars[128];
int var_cnt = 0;

int get_var(char *name) {
  for (int i = 0; i < var_cnt; i++) {
    if (tgc_strcmp(vars[i].name, name) == 0)
      return i;
  }
  tgc_strncpy(vars[var_cnt].name, name, sizeof(vars[var_cnt].name));
  vars[var_cnt].val = 0;
  return var_cnt++;
}

void tgc_lex(char *s) {
  token_cnt = 0;
  while (*s) {
    if (*s <= ' ') {
      s++;
      continue;
    }
    // C95/Standard C Comments
    if (*s == '/' && *(s + 1) == '/') {
      while (*s && *s != '\n')
        s++;
      continue;
    }
    if (*s == '/' && *(s + 1) == '*') {
      s += 2;
      while (*s && !(*s == '*' && *(s + 1) == '/'))
        s++;
      if (*s)
        s += 2;
      continue;
    }

    if (tgc_is_digit(*s)) {
      int v = 0;
      while (tgc_is_digit(*s))
        v = v * 10 + (*s++ - '0');
      tokens[token_cnt].type = T_NUM;
      tokens[token_cnt++].val = v;
    } else if (tgc_is_alpha(*s)) {
      char b[32];
      int i = 0;
      while ((tgc_is_alpha(*s) || tgc_is_digit(*s)) && i < 31) {
        b[i++] = *s++;
      }
      while (tgc_is_alpha(*s) || tgc_is_digit(*s))
        s++;
      b[i] = 0;
      if (tgc_strcmp(b, "int") == 0)
        tokens[token_cnt++].type = T_INT;
      else if (tgc_strcmp(b, "if") == 0)
        tokens[token_cnt++].type = T_IF;
      else if (tgc_strcmp(b, "else") == 0) { /* Handled in stmt */
        tokens[token_cnt++].type = T_ID;
        tgc_strncpy(tokens[token_cnt - 1].str, "else", 32);
        tokens[token_cnt - 1].type = T_ID;
      } // Hack for simple parser
      else if (tgc_strcmp(b, "while") == 0)
        tokens[token_cnt++].type = T_WHILE;
      else if (tgc_strcmp(b, "print") == 0)
        tokens[token_cnt++].type = T_PRINT;
      else if (tgc_strcmp(b, "return") == 0)
        tokens[token_cnt++].type = T_RETURN;
      else if (tgc_strcmp(b, "void") == 0)
        tokens[token_cnt++].type = T_INT; // Map void to int for simplicity
      else {
        tokens[token_cnt].type = T_ID;
        tgc_strncpy(tokens[token_cnt].str, b, sizeof(tokens[token_cnt].str));
        token_cnt++;
      }
    } else if (*s == '"') {
      s++;
      char b[256];
      int i = 0;
      while (*s && *s != '"' && i < 255) {
        if (*s == '\\' && *(s + 1) == 'n') {
          if (i < 254) {
            b[i++] = '\n';
            s += 2;
          } else
            break;
        } else
          b[i++] = *s++;
      }
      if (*s == '"')
        s++;
      b[i] = 0;
      tokens[token_cnt].type = T_STRING;
      tgc_strncpy(tokens[token_cnt++].str, b, 256);
    } else {
      if (*s == '=' && *(s + 1) == '=') {
        tokens[token_cnt++].type = T_EQ;
        s += 2;
      } else if (*s == '=') {
        tokens[token_cnt++].type = T_ASSIGN;
        s++;
      } else if (*s == '+') {
        tokens[token_cnt++].type = T_PLUS;
        s++;
      } else if (*s == '-') {
        tokens[token_cnt++].type = T_MINUS;
        s++;
      } else if (*s == '*') {
        tokens[token_cnt++].type = T_MUL;
        s++;
      } else if (*s == '/') {
        tokens[token_cnt++].type = T_DIV;
        s++;
      } else if (*s == '<') {
        tokens[token_cnt++].type = T_LT;
        s++;
      } else if (*s == '>') {
        tokens[token_cnt++].type = T_GT;
        s++;
      } else if (*s == '(') {
        tokens[token_cnt++].type = T_LPAREN;
        s++;
      } else if (*s == ')') {
        tokens[token_cnt++].type = T_RPAREN;
        s++;
      } else if (*s == '{') {
        tokens[token_cnt++].type = T_LBRACE;
        s++;
      } else if (*s == '}') {
        tokens[token_cnt++].type = T_RBRACE;
        s++;
      } else if (*s == ';') {
        tokens[token_cnt++].type = T_SEMI;
        s++;
      } else
        s++;
    }
  }
  tokens[token_cnt].type = T_EOF;
}

int tgc_expr();
int tgc_stmt(int *rv);
#include "libc.h"

int tgc_call_builtin(char *name, int *args, int arg_cnt) {
  if (tgc_strcmp(name, "strlen") == 0)
    return (int)strlen((char *)(uint64_t)args[0]);
  if (tgc_strcmp(name, "strcmp") == 0)
    return strcmp((char *)(uint64_t)args[0], (char *)(uint64_t)args[1]);
  if (tgc_strcmp(name, "strncmp") == 0)
    return strncmp((char *)(uint64_t)args[0], (char *)(uint64_t)args[1],
                   (size_t)args[2]);
  if (tgc_strcmp(name, "strcpy") == 0)
    return (int)(uint64_t)strcpy((char *)(uint64_t)args[0],
                                 (char *)(uint64_t)args[1]);
  if (tgc_strcmp(name, "strcat") == 0)
    return (int)(uint64_t)strcat((char *)(uint64_t)args[0],
                                 (char *)(uint64_t)args[1]);
  if (tgc_strcmp(name, "strstr") == 0)
    return (int)(uint64_t)strstr((char *)(uint64_t)args[0],
                                 (char *)(uint64_t)args[1]);
  if (tgc_strcmp(name, "abs") == 0)
    return abs(args[0]);
  if (tgc_strcmp(name, "isdigit") == 0)
    return isdigit(args[0]);
  if (tgc_strcmp(name, "isalpha") == 0)
    return isalpha(args[0]);
  if (tgc_strcmp(name, "isalnum") == 0)
    return isalnum(args[0]);
  if (tgc_strcmp(name, "toupper") == 0)
    return toupper(args[0]);
  if (tgc_strcmp(name, "tolower") == 0)
    return tolower(args[0]);
  if (tgc_strcmp(name, "atoi") == 0)
    return atoi((char *)(uint64_t)args[0]);
  if (tgc_strcmp(name, "malloc") == 0)
    return (int)(uint64_t)malloc((size_t)args[0]);
  if (tgc_strcmp(name, "free") == 0) {
    free((void *)(uint64_t)args[0]);
    return 0;
  }
  if (tgc_strcmp(name, "rand") == 0)
    return rand();
  if (tgc_strcmp(name, "srand") == 0) {
    srand((unsigned int)args[0]);
    return 0;
  }
  if (tgc_strcmp(name, "putchar") == 0)
    return putchar(args[0]);
  if (tgc_strcmp(name, "puts") == 0)
    return puts((char *)(uint64_t)args[0]);
  if (tgc_strcmp(name, "printf") == 0) {
    // Limited printf: support up to 3 args for now
    if (arg_cnt == 1)
      return printf((char *)(uint64_t)args[0]);
    if (arg_cnt == 2)
      return printf((char *)(uint64_t)args[0], args[1]);
    if (arg_cnt == 3)
      return printf((char *)(uint64_t)args[0], args[1], args[2]);
    if (arg_cnt == 4)
      return printf((char *)(uint64_t)args[0], args[1], args[2], args[3]);
    return 0;
  }
  return 0;
}

int tgc_factor() {
  if (tokens[t_pos].type == T_NUM)
    return tokens[t_pos++].val;
  if (tokens[t_pos].type == T_ID && tokens[t_pos + 1].type == T_LPAREN) {
    char name[32];
    tgc_strncpy(name, tokens[t_pos].str, 32);
    t_pos += 2;
    int args[8];
    int arg_cnt = 0;
    if (tokens[t_pos].type != T_RPAREN) {
      while (1) {
        args[arg_cnt++] = tgc_expr();
        if (tokens[t_pos].type == T_RPAREN)
          break;
        if (tokens[t_pos].type == T_SEMI)
          break; // Error recovery
        if (tokens[t_pos].type == T_EOF)
          break;
        if (tokens[t_pos].type == T_NUM ||
            tokens[t_pos].type == T_ID) { /* continue */
        } else
          t_pos++; // skip comma if simple parser allowed it
      }
    }
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;

    if (tgc_strcmp(name, "getchar") == 0) {
      if (gui_active) {
        char c = gui_term_getchar();
        if (c == '\n') {
          gui_term_print("\n");
          return '\n';
        }
        char buf[2] = {c, 0};
        gui_term_print(buf);
        return c;
      } else {
        UINTN i;
        EFI_INPUT_KEY k;
        do {
          ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &i);
        } while (ST->ConIn->ReadKeyStroke(ST->ConIn, &k) != EFI_SUCCESS ||
                 k.UnicodeChar == 0);
        if (k.UnicodeChar == '\r') {
          print((CHAR16 *)L"\r\n");
          return '\n';
        }
        print_char(k.UnicodeChar);
        return k.UnicodeChar;
      }
    }
    return tgc_call_builtin(name, args, arg_cnt);
  }
  if (tokens[t_pos].type == T_ID) {
    int idx = get_var(tokens[t_pos++].str);
    return vars[idx].val;
  }
  if (tokens[t_pos].type == T_LPAREN) {
    t_pos++;
    int v = tgc_expr();
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;
    return v;
  }
  return 0;
}
int tgc_term() {
  int v = tgc_factor();
  while (tokens[t_pos].type == T_MUL || tokens[t_pos].type == T_DIV) {
    if (tokens[t_pos].type == T_MUL) {
      t_pos++;
      v *= tgc_factor();
    } else {
      t_pos++;
      int d = tgc_factor();
      if (d != 0)
        v /= d;
      else {
        print((CHAR16 *)L"TGC: Runtime Error - Division by zero\r\n");
      }
    }
  }
  return v;
}
int tgc_expr_add() {
  int v = tgc_term();
  while (tokens[t_pos].type == T_PLUS || tokens[t_pos].type == T_MINUS) {
    if (tokens[t_pos].type == T_PLUS) {
      t_pos++;
      v += tgc_term();
    } else {
      t_pos++;
      v -= tgc_term();
    }
  }
  return v;
}
int tgc_expr() {
  int v = tgc_expr_add();
  if (tokens[t_pos].type == T_LT) {
    t_pos++;
    return v < tgc_expr_add();
  }
  if (tokens[t_pos].type == T_GT) {
    t_pos++;
    return v > tgc_expr_add();
  }
  if (tokens[t_pos].type == T_EQ) {
    t_pos++;
    return v == tgc_expr_add();
  }
  return v;
}

void tgc_skip() {
  int b = 0;
  while (tokens[t_pos].type != T_EOF) {
    if (tokens[t_pos].type == T_LBRACE)
      b++;
    else if (tokens[t_pos].type == T_RBRACE) {
      b--;
      if (b < 0) {
        t_pos++;
        break;
      }
    }
    t_pos++;
  }
}
int tgc_stmt(int *rv) {
  if (tokens[t_pos].type == T_INT) {
    t_pos++;
    if (tokens[t_pos].type == T_ID) {
      int i = get_var(tokens[t_pos++].str);
      if (tokens[t_pos].type == T_ASSIGN) {
        t_pos++;
        vars[i].val = tgc_expr();
      }
    }
    if (tokens[t_pos].type == T_SEMI)
      t_pos++;
  } else if (tokens[t_pos].type == T_ID) {
    char name[32];
    tgc_strncpy(name, tokens[t_pos].str, 32);
    int i = get_var(tokens[t_pos++].str);
    if (tokens[t_pos].type == T_ASSIGN) {
      t_pos++;
      vars[i].val = tgc_expr();
    }
    if (tokens[t_pos].type == T_SEMI)
      t_pos++;
  } else if (tokens[t_pos].type == T_PRINT) {
    t_pos++;
    if (tokens[t_pos].type == T_LPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_STRING) {
      CHAR16 w[256];
      int i = 0, j = 0;
      while (tokens[t_pos].str[i] && j < 254) {
        if (tokens[t_pos].str[i] == '\n') {
          if (j < 253) {
            w[j++] = '\r';
            w[j++] = '\n';
          } else
            break;
          i++;
        } else
          w[j++] = tokens[t_pos].str[i++];
      }
      w[j] = 0;
      print(w);
      t_pos++;
    } else {
      print_uint(tgc_expr());
      print((CHAR16 *)L"\r\n");
    }
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_SEMI)
      t_pos++;
  } else if (tokens[t_pos].type == T_RETURN) {
    t_pos++;
    *rv = tgc_expr();
    if (tokens[t_pos].type == T_SEMI)
      t_pos++;
    return 1;
  } else if (tokens[t_pos].type == T_IF) {
    t_pos++;
    if (tokens[t_pos].type == T_LPAREN)
      t_pos++;
    int c = tgc_expr();
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_LBRACE)
      t_pos++;
    if (c) {
      while (tokens[t_pos].type != T_RBRACE && tokens[t_pos].type != T_EOF) {
        if (tgc_stmt(rv))
          return 1;
      }
      if (tokens[t_pos].type == T_RBRACE)
        t_pos++;
      // Skip else if present
      if (tokens[t_pos].type == T_ID &&
          tgc_strcmp(tokens[t_pos].str, "else") == 0) {
        t_pos++;
        if (tokens[t_pos].type == T_LBRACE)
          tgc_skip();
        else
          tgc_stmt(rv);
      }
    } else {
      tgc_skip();
      if (tokens[t_pos].type == T_ID &&
          tgc_strcmp(tokens[t_pos].str, "else") == 0) {
        t_pos++;
        if (tokens[t_pos].type == T_LBRACE)
          t_pos++;
        while (tokens[t_pos].type != T_RBRACE && tokens[t_pos].type != T_EOF) {
          if (tgc_stmt(rv))
            return 1;
        }
        if (tokens[t_pos].type == T_RBRACE)
          t_pos++;
      }
    }
  } else if (tokens[t_pos].type == T_WHILE) {
    int s = t_pos;
    t_pos++;
    if (tokens[t_pos].type == T_LPAREN)
      t_pos++;
    int c = tgc_expr();
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_LBRACE)
      t_pos++;
    if (c) {
      while (tokens[t_pos].type != T_RBRACE && tokens[t_pos].type != T_EOF) {
        if (tgc_stmt(rv))
          return 1;
      }
      if (tokens[t_pos].type == T_RBRACE)
        t_pos++;
      t_pos = s;
    } else
      tgc_skip();
  } else if (tokens[t_pos].type == T_LBRACE) {
    t_pos++;
    while (tokens[t_pos].type != T_RBRACE && tokens[t_pos].type != T_EOF) {
      if (tgc_stmt(rv))
        return 1;
    }
    if (tokens[t_pos].type == T_RBRACE)
      t_pos++;
  } else if (tokens[t_pos].type == T_STRING) { /* HolyC Print */
    CHAR16 w[256];
    int i = 0, j = 0;
    while (tokens[t_pos].str[i] && j < 254) {
      if (tokens[t_pos].str[i] == '\n') {
        if (j < 253) {
          w[j++] = '\r';
          w[j++] = '\n';
        } else
          break;
        i++;
      } else
        w[j++] = tokens[t_pos].str[i++];
    }
    w[j] = 0;
    print(w);
    t_pos++;
    if (tokens[t_pos].type == T_SEMI)
      t_pos++;
  } else {
    if (tokens[t_pos].type != T_EOF) {
      print((CHAR16 *)L"TGC: Error - Unexpected token at pos ");
      print_uint(t_pos);
      print((CHAR16 *)L"\r\n");
      t_pos++;
    }
  }
  return 0;
}

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint64_t entry;
  uint64_t flags;
  uint64_t ph_off;
  uint16_t ph_cnt;
  uint64_t sh_off;
  uint8_t pad[14];
} texf_header_t;

void cmd_tgc(CHAR16 *args) {
  CHAR16 *filename = args;
  int is_hc = 0;
  if (strncmp16(args, (CHAR16 *)L"-H ", 3) == 0) {
    is_hc = 1;
    filename = &args[3];
  }
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  EFI_FILE_PROTOCOL *f;
  if (dir->Open(dir, &f, filename, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) {
    print((CHAR16 *)L"TGC: File not found\r\n");
    return;
  }
  UINTN sz = 8192;
  char *b;
  ST->BootServices->AllocatePool(2, sz, (void **)&b);
  f->Read(f, &sz, b);
  b[sz] = 0;
  f->Close(f);
  print((CHAR16 *)L"TGC: Compiling ");
  print(filename);
  print((CHAR16 *)L"...\r\n");
  var_cnt = 0;
  tgc_lex(b);
  int mp = -1;
  for (int i = 0; i < token_cnt; i++)
    if (tokens[i].type == T_ID && tgc_strcmp(tokens[i].str, "main") == 0) {
      mp = i;
      break;
    }
  t_pos = 0;
  if (mp != -1) {
    t_pos = mp + 1;
    if (tokens[t_pos].type == T_LPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_RPAREN)
      t_pos++;
    if (tokens[t_pos].type == T_LBRACE)
      t_pos++;
  }
  int rv = 0;
  while (tokens[t_pos].type != T_EOF && tokens[t_pos].type != T_RBRACE) {
    if (tgc_stmt(&rv))
      break;
  }
  CHAR16 bin[256];
  strncpy16(bin, filename, 256);
  int bl = strlen16(bin);
  if (bl > 2) {
    bin[bl - 1] = 'f';
    bin[bl - 2] = 'x';
  }
  EFI_FILE_PROTOCOL *bf;
  if (dir->Open(dir, &bf, bin,
                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                0) == EFI_SUCCESS) {
    texf_header_t h = {0x46584554, 1, 0x401000, is_hc ? 2 : 1, 0, 0, 0, {0}};
    UINTN hs = sizeof(h);
    bf->Write(bf, &hs, &h);
    bf->Close(bf);
    print((CHAR16 *)L"Binary generated: ");
    print(bin);
    print((CHAR16 *)L"\r\n");
  }
  ST->BootServices->FreePool(b);
  print((CHAR16 *)L"Process exited with code: ");
  print_uint(rv);
  print((CHAR16 *)L"\r\n");
  if (dir != cwd)
    dir->Close(dir);
}

void cmd_asm(char *instr) {
  uint8_t code[16];
  int len = 0;
  if (tgc_strcmp(instr, "ret") == 0)
    code[len++] = 0xC3;
  else if (tgc_strcmp(instr, "mov rax, 42") == 0) {
    code[len++] = 0x48;
    code[len++] = 0xC7;
    code[len++] = 0xC0;
    code[len++] = 42;
    code[len++] = 0;
    code[len++] = 0;
    code[len++] = 0;
  }
  if (len > 0) {
    print((CHAR16 *)L"Encoded: ");
    for (int i = 0; i < len; i++) {
      UINTN b = code[i];
      const CHAR16 *h = L"0123456789ABCDEF";
      print_char(h[b >> 4]);
      print_char(h[b & 0xF]);
      print_char(' ');
    }
    print((CHAR16 *)L"\r\n");
  } else
    print((CHAR16 *)L"Unknown instr\r\n");
}

void autocomplete(CHAR16 *b, UINTN *l) {
  CHAR16 *last = b;
  for (UINTN i = 0; i < *l; i++)
    if (b[i] == ' ')
      last = &b[i + 1];
  UINTN wl = strlen16(last);
  if (wl == 0)
    return;
  const CHAR16 *cs[] = {L"ls",     L"cd",       L"mkdir", L"tgo", L"tgc",
                        L"pstree", L"shutdown", L"help",  NULL};
  for (int i = 0; cs[i]; i++)
    if (strncmp16(last, (CHAR16 *)cs[i], wl) == 0) {
      const CHAR16 *m = cs[i] + wl;
      while (*m) {
        b[(*l)++] = *m;
        print_char(*m++);
      }
      b[*l] = 0;
      return;
    }
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  dir->SetPosition(dir, 0);
  EFI_FILE_INFO *info;
  UINTN is = sizeof(EFI_FILE_INFO) + 256;
  ST->BootServices->AllocatePool(2, is, (void **)&info);
  while (1) {
    UINTN cs = is;
    if (dir->Read(dir, &cs, info) != EFI_SUCCESS || cs == 0)
      break;
    if (strncmp16(last, info->FileName, wl) == 0) {
      CHAR16 *m = info->FileName + wl;
      while (*m) {
        b[(*l)++] = *m;
        print_char(*m++);
      }
      b[*l] = 0;
      break;
    }
  }
  ST->BootServices->FreePool(info);
  if (dir != cwd)
    dir->Close(dir);
}

void cmd_ls() {
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  dir->SetPosition(dir, 0);
  EFI_FILE_INFO *info;
  UINTN bs = sizeof(EFI_FILE_INFO) + 256;
  ST->BootServices->AllocatePool(2, bs, (void **)&info);
  while (1) {
    UINTN cs = bs;
    if (dir->Read(dir, &cs, info) != EFI_SUCCESS || cs == 0)
      break;
    if (info->Attribute & EFI_FILE_DIRECTORY)
      print((CHAR16 *)L"[DIR] ");
    else
      print((CHAR16 *)L"      ");
    print(info->FileName);
    print((CHAR16 *)L"\r\n");
  }
  ST->BootServices->FreePool(info);
  if (dir != cwd)
    dir->Close(dir);
}

void cmd_cd(CHAR16 *p) {
  if (strcmp16(p, (CHAR16 *)L"/") == 0) {
    if (cwd)
      cwd->Close(cwd);
    get_root(&cwd);
    return;
  }
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  EFI_FILE_PROTOCOL *nd;
  if (dir->Open(dir, &nd, p, EFI_FILE_MODE_READ, 0) == EFI_SUCCESS) {
    if (cwd && cwd != dir)
      cwd->Close(cwd);
    cwd = nd;
  } else
    print((CHAR16 *)L"Not found.\r\n");
}
void cmd_mkdir(CHAR16 *n) {
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  EFI_FILE_PROTOCOL *nd;
  if (dir->Open(dir, &nd, n,
                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                EFI_FILE_DIRECTORY) == EFI_SUCCESS) {
    print((CHAR16 *)L"Created.\r\n");
    nd->Close(nd);
  } else
    print((CHAR16 *)L"Failed.\r\n");
  if (dir != cwd)
    dir->Close(dir);
}

void cmd_run(CHAR16 *filename) {
  EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL;
  if (!dir)
    get_root(&dir);
  EFI_FILE_PROTOCOL *f;
  if (dir->Open(dir, &f, filename, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) {
    print((CHAR16 *)L"Run: File not found\r\n");
    return;
  }

  texf_header_t hdr;
  UINTN hsz = sizeof(hdr);
  f->Read(f, &hsz, &hdr);
  if (hdr.magic != 0x46584554) {
    print((CHAR16 *)L"Run: Invalid TEXF magic\r\n");
    f->Close(f);
    return;
  }

  print((CHAR16 *)L"Running binary...\r\n");
  // In a real OS we would map segments. For now, we simulate execution.
  // Since our compiler currently interprets, the .texf is just a metadata
  // container. To match the prompt, we would jump to hdr.entry.
  print((CHAR16 *)L"Execution simulation successful.\r\n");
  f->Close(f);
}

void cmd_about() {
  print((CHAR16 *)L"  _______      ____   _____ \r\n");
  print((CHAR16 *)L" |__   __|    / __ \\ / ____|\r\n");
  print((CHAR16 *)L"    | |      | |  | | (___  \r\n");
  print((CHAR16 *)L"    | |      | |  | |\\___ \\ \r\n");
  print((CHAR16 *)L"    | |      | |__| |____) |\r\n");
  print((CHAR16 *)L"    |_|       \\____/|_____/ \r\n\r\n");
  print((CHAR16 *)L" T-OS VERSION 0.1 ALPHA A\r\n");
  print((CHAR16 *)L" Built via TGC Compiler\r\n");
  print((CHAR16 *)L" Target: x86_64 UEFI\r\n\r\n");
}

void cmd_panic() {
  if (global_boot_info.fb_base != 0) {
    // Clear screen to red for panic
    uint32_t *fb = (uint32_t *)global_boot_info.fb_base;
    uint32_t num_pixels = global_boot_info.fb_size / 4;
    for (uint32_t i = 0; i < num_pixels; i++) {
      fb[i] = 0x00FF0000; // Red color
    }

    uint32_t screen_center_x = global_boot_info.fb_width / 2;
    uint32_t screen_center_y = global_boot_info.fb_height / 2;

    uint32_t panic_len = 5 * 8 * 5; // 5 chars, 8 pixels wide, 5x scale
    font_draw_string(&global_boot_info, "PANIC",
                     screen_center_x - panic_len / 2, screen_center_y,
                     0xFFFFFFFF, 5); // White text

    // Wait for a key press before returning to console
    print((CHAR16 *)L"\r\nPress any key to return to shell...");
    UINTN idx;
    EFI_INPUT_KEY k;
    ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &idx);
    ST->ConIn->ReadKeyStroke(ST->ConIn, &k); // Consume the key press

    // Clear screen and reset console attributes
    ST->ConOut->ClearScreen(ST->ConOut);
    ST->ConOut->SetAttribute(ST->ConOut,
                             EFI_TEXT_ATTRIBUTE(EFI_LIGHTGRAY, EFI_BLACK));
    ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
  } else {
    print((CHAR16 *)L"Graphics not available for panic display.\r\n");
  }
}

void cmd_startgui() {
  if (global_boot_info.fb_base == 0) {
    print((CHAR16 *)L"Graphics output is not available.\r\n");
    return;
  }

  gui_init(ST, global_boot_info);
  gui_active = 1;
  gui_start();

  print((CHAR16 *)L"Welcome to T-OS GUI Terminal!\r\n");
  print((CHAR16 *)L"Type 'exit' to return to text mode.\r\n");

  char cmd_buf[256];
  int cmd_len = 0;

  while (gui_active) {
    print((CHAR16 *)L"T-OS-> ");
    cmd_len = 0;
    while (1) {
      char c = gui_term_getchar();
      if (c == '\n') {
        print((CHAR16 *)L"\r\n");
        break;
      } else if (c == '\b' && cmd_len > 0) {
        cmd_len--;
        print((CHAR16 *)L"\b \b");
      } else if (c >= 32 && c <= 126 && cmd_len < 254) {
        cmd_buf[cmd_len++] = c;
        CHAR16 w[2] = {c, 0};
        print(w);
      }
    }
    cmd_buf[cmd_len] = 0;

    if (tgc_strcmp(cmd_buf, "exit") == 0) {
      gui_active = 0;
      break;
    }

    // Execute command
    CHAR16 wcmd[256];
    for (int i = 0; i <= cmd_len; i++)
      wcmd[i] = (CHAR16)cmd_buf[i];
    execute_command(wcmd);
  }

  ST->ConOut->ClearScreen(ST->ConOut);
}

void delay_ms(UINTN milliseconds) {
  // UEFI Stall is in microseconds, so multiply by 1000
  ST->BootServices->Stall(milliseconds * 1000);
}

void execute_command(CHAR16 *cmd) {
  if (cmd[0] == 0)
    return;
  Process *p = create_process(current_proc, cmd);
  char acmd[256];
  int i;
  for (i = 0; i < 255; i++) {
    acmd[i] = (char)cmd[i];
    if (cmd[i] == 0)
      break;
  }
  acmd[i] = 0;
  acmd[255] = 0;

  if (strcmp16(cmd, (CHAR16 *)L"help") == 0)
    print((CHAR16 *)L"ls, cd, mkdir, tgo, tgc, run, asm, pstree, about, "
                    L"shutdown, panic, startgui\r\n");
  else if (strcmp16(cmd, (CHAR16 *)L"about") == 0)
    cmd_about();
  else if (strcmp16(cmd, (CHAR16 *)L"ls") == 0)
    cmd_ls();
  else if (strncmp16(cmd, (CHAR16 *)L"cd ", 3) == 0)
    cmd_cd(&cmd[3]);
  else if (strncmp16(cmd, (CHAR16 *)L"mkdir ", 6) == 0)
    cmd_mkdir(&cmd[6]);
  else if (strncmp16(cmd, (CHAR16 *)L"tgo ", 4) == 0)
    cmd_tgo(&cmd[4]);
  else if (strncmp16(cmd, (CHAR16 *)L"tgc ", 4) == 0)
    cmd_tgc(&cmd[4]);
  else if (strncmp16(cmd, (CHAR16 *)L"run ", 4) == 0)
    cmd_run(&cmd[4]);
  else if (strncmp16(cmd, (CHAR16 *)L"asm ", 4) == 0)
    cmd_asm(&acmd[4]);
  else if (strcmp16(cmd, (CHAR16 *)L"pstree") == 0)
    print_pstree(&proc_table[0], 0);
  else if (strcmp16(cmd, (CHAR16 *)L"shutdown") == 0)
    ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
  else if (strcmp16(cmd, (CHAR16 *)L"panic") == 0)
    cmd_panic();
  else if (strcmp16(cmd, (CHAR16 *)L"startgui") == 0)
    cmd_startgui();
  else
    print((CHAR16 *)L"Unknown command.\r\n");
  if (p)
    p->status = PROC_EXITED;
}

void show_splash_screen() {
  if (global_boot_info.fb_base == 0)
    return;
  uint32_t *fb = (uint32_t *)global_boot_info.fb_base;
  uint32_t w = global_boot_info.fb_width;
  uint32_t h = global_boot_info.fb_height;

  // Clear to dark blue
  for (uint32_t i = 0; i < global_boot_info.fb_size / 4; i++)
    fb[i] = 0x00001A33;

  uint32_t mid_x = w / 2;
  uint32_t mid_y = h / 2;

  // "T-OS" Big Text
  font_draw_string(&global_boot_info, "T-OS", mid_x - (4 * 8 * 10) / 2,
                   mid_y - 40, 0x00FFFFFF, 10);
  font_draw_string(&global_boot_info, "Version 0.1 Alpha B",
                   mid_x - (19 * 8 * 2) / 2, mid_y + 80, 0x00CCCCCC, 2);
  font_draw_string(&global_boot_info, "Loading System Components...",
                   mid_x - (28 * 8 * 1) / 2, mid_y + 120, 0x00AAAAAA, 1);

  ST->BootServices->Stall(SPLASH_STALL); // 2 second splash
  ST->ConOut->ClearScreen(ST->ConOut);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable) {
  (void)ImageHandle;
  ST = SystemTable;
  ImgHandle = ImageHandle;
  current_proc = create_process(NULL, (CHAR16 *)L"t-shell");
  get_root(&cwd);

  // Get Graphics Output Protocol
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&gop);

  if (gop && gop->Mode && gop->Mode->Info) {
    global_boot_info.fb_base = gop->Mode->FrameBufferBase;
    global_boot_info.fb_size = gop->Mode->FrameBufferSize;
    global_boot_info.fb_width = gop->Mode->Info->HorizontalResolution;
    global_boot_info.fb_height = gop->Mode->Info->VerticalResolution;
    global_boot_info.fb_pitch = gop->Mode->Info->PixelsPerScanLine;
    if (global_boot_info.fb_pitch == 0)
      global_boot_info.fb_pitch = global_boot_info.fb_width;
  }

  show_splash_screen();
  ST->ConOut->ClearScreen(ST->ConOut);
  ST->ConOut->EnableCursor(ST->ConOut, TRUE);
  cmd_about();
  print((CHAR16 *)L"Welcome to T-OS! Type 'help' for commands.\r\n");
  CHAR16 b[256];
  UINTN l = 0;
  while (1) {
    print((CHAR16 *)L"T-OS-> ");
    l = 0;
    b[0] = 0;
    while (1) {
      UINTN i;
      EFI_INPUT_KEY k;
      ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &i);
      if (ST->ConIn->ReadKeyStroke(ST->ConIn, &k) == EFI_SUCCESS) {
        if (k.UnicodeChar == '\r') {
          print((CHAR16 *)L"\r\n");
          break;
        } else if (k.UnicodeChar == '\b' && l > 0) {
          l--;
          print((CHAR16 *)L"\b \b");
        } else if (k.UnicodeChar == '\t')
          autocomplete(b, &l);
        else if (k.UnicodeChar != 0 && l < 254) {
          b[l++] = k.UnicodeChar;
          print_char(k.UnicodeChar);
        }
      }
    }
    b[l] = 0;
    execute_command(b);
  }
  return EFI_SUCCESS;
}
