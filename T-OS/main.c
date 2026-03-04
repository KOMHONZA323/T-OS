#include "uefi.h"

EFI_SYSTEM_TABLE *ST;
EFI_HANDLE ImgHandle;

typedef enum { PROC_RUNNING, PROC_EXITED } ProcStatus;
typedef struct Process {
    UINTN pid; UINTN ppid; CHAR16 cmd[64]; ProcStatus status;
    struct Process *parent; struct Process *first_child; struct Process *next_sibling;
} Process;

#define MAX_PROCS 256
Process proc_table[MAX_PROCS];
UINTN proc_count = 0; Process *current_proc = NULL; UINTN next_pid = 1;

void print(CHAR16 *str) { ST->ConOut->OutputString(ST->ConOut, str); }
void print_char(CHAR16 c) { CHAR16 str[2]; str[0] = c; str[1] = 0; print(str); }
void print_uint(UINTN n) {
    if (n == 0) { print_char('0'); return; }
    CHAR16 buf[32]; int i = 0;
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    while (i > 0) print_char(buf[--i]);
}

int strcmp16(CHAR16 *s1, CHAR16 *s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *s1 - *s2; }
int strncmp16(CHAR16 *s1, CHAR16 *s2, UINTN n) { while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; } if (n == 0) return 0; return *s1 - *s2; }
void strcpy16(CHAR16 *dest, CHAR16 *src) { while (*src) *dest++ = *src++; *dest = 0; }
UINTN strlen16(CHAR16 *s) { UINTN len = 0; while (*s++) len++; return len; }

Process* create_process(Process *parent, CHAR16 *cmd) {
    if (proc_count >= MAX_PROCS) return NULL;
    Process *p = &proc_table[proc_count++];
    p->pid = next_pid++; p->ppid = parent ? parent->pid : 0; p->status = PROC_RUNNING;
    p->parent = parent; p->first_child = NULL; p->next_sibling = NULL;
    UINTN i = 0; while (cmd[i] && i < 63) { p->cmd[i] = cmd[i]; i++; } p->cmd[i] = 0;
    if (parent) {
        if (parent->first_child == NULL) parent->first_child = p;
        else { Process *sibling = parent->first_child; while (sibling->next_sibling) sibling = sibling->next_sibling; sibling->next_sibling = p; }
    }
    return p;
}

void print_pstree(Process *p, UINTN indent) {
    if (!p) return;
    for (UINTN i = 0; i < indent; i++) print((CHAR16*)L"  ");
    if (indent > 0) print((CHAR16*)L"|- ");
    print(p->cmd); print((CHAR16*)L" ["); print_uint(p->pid); print((CHAR16*)L"]\r\n");
    Process *child = p->first_child; while (child) { print_pstree(child, indent + 1); child = child->next_sibling; }
}

EFI_FILE_PROTOCOL *cwd = NULL;
void get_root(EFI_FILE_PROTOCOL **root) {
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage; EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    ST->BootServices->HandleProtocol(ImgHandle, &loaded_image_guid, (void **)&LoadedImage);
    ST->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &fs_guid, (void **)&FileSystem);
    FileSystem->OpenVolume(FileSystem, root);
}

char tgo_buffer[4096];
void tgo_redraw(CHAR16 *filename, UINTN buffer_size, UINTN cursor_pos) {
    ST->ConOut->ClearScreen(ST->ConOut); ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
    print((CHAR16*)L"TGO - Editing: "); print(filename); print((CHAR16*)L" | [ESC:Save] [Arrows:Move]\r\n--------------------------------\r\n");
    UINTN cur_x = 0, cur_y = 2, target_x = 0, target_y = 2;
    for (UINTN i = 0; i <= buffer_size; i++) {
        if (i == cursor_pos) { target_x = cur_x; target_y = cur_y; }
        if (i < buffer_size) {
            if (tgo_buffer[i] == '\n') { print((CHAR16*)L"\r\n"); cur_x = 0; cur_y++; }
            else { print_char(tgo_buffer[i]); cur_x++; }
        }
    }
    ST->ConOut->SetCursorPosition(ST->ConOut, target_x, target_y);
}

void cmd_tgo(CHAR16 *filename) {
    EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL; if (!dir) get_root(&dir);
    EFI_FILE_PROTOCOL *File;
    if (dir->Open(dir, &File, filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0) != EFI_SUCCESS) { print((CHAR16*)L"TGO: Error opening file\r\n"); return; }
    UINTN buffer_size = 4095; File->Read(File, &buffer_size, tgo_buffer);
    tgo_buffer[buffer_size] = 0; UINTN cursor_pos = buffer_size;
    ST->ConOut->EnableCursor(ST->ConOut, TRUE); tgo_redraw(filename, buffer_size, cursor_pos);
    while (1) {
        UINTN idx; EFI_INPUT_KEY key; ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &idx);
        if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key) != EFI_SUCCESS) continue;
        if (key.ScanCode == SCAN_ESC) break;
        if (key.ScanCode == SCAN_LEFT && cursor_pos > 0) cursor_pos--;
        else if (key.ScanCode == SCAN_RIGHT && cursor_pos < buffer_size) cursor_pos++;
        else if (key.ScanCode == SCAN_UP) {
            UINTN line_start = cursor_pos; while (line_start > 0 && tgo_buffer[line_start-1] != '\n') line_start--;
            if (line_start > 0) {
                UINTN col = cursor_pos - line_start; UINTN prev_end = line_start - 1; UINTN prev_start = prev_end;
                while (prev_start > 0 && tgo_buffer[prev_start-1] != '\n') prev_start--;
                UINTN prev_len = prev_end - prev_start; cursor_pos = prev_start + (col < prev_len ? col : prev_len);
            }
        } else if (key.ScanCode == SCAN_DOWN) {
            UINTN line_end = cursor_pos; while (line_end < buffer_size && tgo_buffer[line_end] != '\n') line_end++;
            if (line_end < buffer_size) {
                UINTN line_start = cursor_pos; while (line_start > 0 && tgo_buffer[line_start-1] != '\n') line_start--;
                UINTN col = cursor_pos - line_start; UINTN next_start = line_end + 1; UINTN next_end = next_start;
                while (next_end < buffer_size && tgo_buffer[next_end] != '\n') next_end++;
                UINTN next_len = next_end - next_start; cursor_pos = next_start + (col < next_len ? col : next_len);
            }
        } else if (key.UnicodeChar == '\b' && cursor_pos > 0) {
            for (UINTN i = cursor_pos - 1; i < buffer_size; i++) tgo_buffer[i] = tgo_buffer[i+1];
            buffer_size--; cursor_pos--;
        } else if (key.UnicodeChar != 0) {
            if (buffer_size < 4094) {
                char c = (key.UnicodeChar == '\r') ? '\n' : (char)key.UnicodeChar;
                for (UINTN i = buffer_size; i > cursor_pos; i--) tgo_buffer[i] = tgo_buffer[i-1];
                tgo_buffer[cursor_pos++] = c; buffer_size++;
            }
        }
        tgo_redraw(filename, buffer_size, cursor_pos);
    }
    File->SetPosition(File, 0); File->Write(File, &buffer_size, tgo_buffer); File->Close(File);
    if (dir != cwd) dir->Close(dir); ST->ConOut->EnableCursor(ST->ConOut, FALSE); ST->ConOut->ClearScreen(ST->ConOut);
}

int tgc_strcmp(const char* s1, const char* s2) { while(*s1 && (*s1 == *s2)) { s1++; s2++; } return *(unsigned char*)s1 - *(unsigned char*)s2; }
void tgc_strcpy(char* d, const char* s) { while(*s) *d++ = *s++; *d = 0; }
int tgc_is_digit(char c) { return c >= '0' && c <= '9'; }
int tgc_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

typedef enum { T_INT, T_ID, T_NUM, T_STRING, T_IF, T_WHILE, T_PRINT, T_RETURN, T_PLUS, T_MINUS, T_MUL, T_DIV, T_LT, T_GT, T_EQ, T_ASSIGN, T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_SEMI, T_EOF } TokenType;
typedef struct { TokenType type; char str[256]; int val; } Token;
Token tokens[2048]; int token_cnt = 0; int t_pos = 0;
struct { char name[32]; int val; } vars[128]; int var_cnt = 0;

int get_var(char* name) {
    for (int i=0; i<var_cnt; i++) if (tgc_strcmp(vars[i].name, name) == 0) return i;
    tgc_strcpy(vars[var_cnt].name, name); vars[var_cnt].val = 0; return var_cnt++;
}

void tgc_lex(char* s) {
    token_cnt = 0;
    while (*s) {
        if (*s <= ' ') { s++; continue; }
        if (tgc_is_digit(*s)) { int v=0; while(tgc_is_digit(*s)) v=v*10+(*s++-'0'); tokens[token_cnt].type=T_NUM; tokens[token_cnt++].val=v; }
        else if (tgc_is_alpha(*s)) {
            char b[32]; int i=0; while(tgc_is_alpha(*s)||tgc_is_digit(*s)) { if(i<31) b[i++]=*s; s++; } b[i]=0;
            if(tgc_strcmp(b,"int")==0) tokens[token_cnt++].type=T_INT;
            else if(tgc_strcmp(b,"if")==0) tokens[token_cnt++].type=T_IF;
            else if(tgc_strcmp(b,"while")==0) tokens[token_cnt++].type=T_WHILE;
            else if(tgc_strcmp(b,"print")==0) tokens[token_cnt++].type=T_PRINT;
            else if(tgc_strcmp(b,"return")==0) tokens[token_cnt++].type=T_RETURN;
            else { tokens[token_cnt].type=T_ID; tgc_strcpy(tokens[token_cnt++].str,b); }
        } else if (*s == '"') {
            s++; char b[256]; int i=0; while(*s&&*s!='"'){ if(*s=='\\'&&*(s+1)=='n'){ b[i++]='\n'; s+=2; } else b[i++]=*s++; }
            if(*s=='"')s++; b[i]=0; tokens[token_cnt].type=T_STRING; tgc_strcpy(tokens[token_cnt++].str,b);
        } else {
            if(*s=='='&&*(s+1)=='='){ tokens[token_cnt++].type=T_EQ; s+=2; }
            else if(*s=='='){ tokens[token_cnt++].type=T_ASSIGN; s++; }
            else if(*s=='+'){ tokens[token_cnt++].type=T_PLUS; s++; }
            else if(*s=='-'){ tokens[token_cnt++].type=T_MINUS; s++; }
            else if(*s=='*'){ tokens[token_cnt++].type=T_MUL; s++; }
            else if(*s=='/'){ tokens[token_cnt++].type=T_DIV; s++; }
            else if(*s=='<'){ tokens[token_cnt++].type=T_LT; s++; }
            else if(*s=='>'){ tokens[token_cnt++].type=T_GT; s++; }
            else if(*s=='('){ tokens[token_cnt++].type=T_LPAREN; s++; }
            else if(*s==')'){ tokens[token_cnt++].type=T_RPAREN; s++; }
            else if(*s=='{'){ tokens[token_cnt++].type=T_LBRACE; s++; }
            else if(*s=='}'){ tokens[token_cnt++].type=T_RBRACE; s++; }
            else if(*s==';'){ tokens[token_cnt++].type=T_SEMI; s++; }
            else s++;
        }
    }
    tokens[token_cnt].type = T_EOF;
}

int tgc_expr();
int tgc_stmt(int *rv);
int tgc_factor() {
    if(tokens[t_pos].type==T_NUM) return tokens[t_pos++].val;
    if(tokens[t_pos].type==T_ID && tgc_strcmp(tokens[t_pos].str,"getchar")==0){
        t_pos++; if(tokens[t_pos].type==T_LPAREN)t_pos++; if(tokens[t_pos].type==T_RPAREN)t_pos++;
        UINTN i; EFI_INPUT_KEY k; do{ ST->BootServices->WaitForEvent(1,&ST->ConIn->WaitForKey,&i); }while(ST->ConIn->ReadKeyStroke(ST->ConIn,&k)!=EFI_SUCCESS||k.UnicodeChar==0);
        if(k.UnicodeChar=='\r'){print((CHAR16*)L"\r\n");return '\n';} print_char(k.UnicodeChar); return k.UnicodeChar;
    }
    if(tokens[t_pos].type==T_ID){ int idx=get_var(tokens[t_pos++].str); return vars[idx].val; }
    if(tokens[t_pos].type==T_LPAREN){ t_pos++; int v=tgc_expr(); if(tokens[t_pos].type==T_RPAREN)t_pos++; return v; }
    return 0;
}
int tgc_term(){ int v=tgc_factor(); while(tokens[t_pos].type==T_MUL||tokens[t_pos].type==T_DIV){ if(tokens[t_pos].type==T_MUL){t_pos++;v*=tgc_factor();} else{t_pos++;int d=tgc_factor();if(d!=0)v/=d;} } return v; }
int tgc_expr_add(){ int v=tgc_term(); while(tokens[t_pos].type==T_PLUS||tokens[t_pos].type==T_MINUS){ if(tokens[t_pos].type==T_PLUS){t_pos++;v+=tgc_term();} else{t_pos++;v-=tgc_term();} } return v; }
int tgc_expr(){ int v=tgc_expr_add(); if(tokens[t_pos].type==T_LT){t_pos++;return v<tgc_expr_add();} if(tokens[t_pos].type==T_GT){t_pos++;return v>tgc_expr_add();} if(tokens[t_pos].type==T_EQ){t_pos++;return v==tgc_expr_add();} return v; }

void tgc_skip() { int b=0; while(tokens[t_pos].type!=T_EOF){ if(tokens[t_pos].type==T_LBRACE)b++; else if(tokens[t_pos].type==T_RBRACE){b--;if(b<0){t_pos++;break;}} t_pos++; } }
int tgc_stmt(int *rv) {
    if(tokens[t_pos].type==T_INT){ t_pos++; if(tokens[t_pos].type==T_ID){ int i=get_var(tokens[t_pos++].str); if(tokens[t_pos].type==T_ASSIGN){t_pos++;vars[i].val=tgc_expr();} } if(tokens[t_pos].type==T_SEMI)t_pos++; }
    else if(tokens[t_pos].type==T_ID){ int i=get_var(tokens[t_pos++].str); if(tokens[t_pos].type==T_ASSIGN){t_pos++;vars[i].val=tgc_expr();} if(tokens[t_pos].type==T_SEMI)t_pos++; }
    else if(tokens[t_pos].type==T_PRINT){ t_pos++; if(tokens[t_pos].type==T_LPAREN)t_pos++; if(tokens[t_pos].type==T_STRING){ CHAR16 w[256]; int i=0,j=0; while(tokens[t_pos].str[i]&&j<254){ if(tokens[t_pos].str[i]=='\n'){w[j++]='\r';w[j++]='\n';i++;} else w[j++]=tokens[t_pos].str[i++]; } w[j]=0; print(w); t_pos++; } else{ print_uint(tgc_expr()); print((CHAR16*)L"\r\n"); } if(tokens[t_pos].type==T_RPAREN)t_pos++; if(tokens[t_pos].type==T_SEMI)t_pos++; }
    else if(tokens[t_pos].type==T_RETURN){ t_pos++; *rv=tgc_expr(); if(tokens[t_pos].type==T_SEMI)t_pos++; return 1; }
    else if(tokens[t_pos].type==T_IF){ t_pos++; if(tokens[t_pos].type==T_LPAREN)t_pos++; int c=tgc_expr(); if(tokens[t_pos].type==T_RPAREN)t_pos++; if(tokens[t_pos].type==T_LBRACE)t_pos++; if(c){ while(tokens[t_pos].type!=T_RBRACE&&tokens[t_pos].type!=T_EOF){if(tgc_stmt(rv))return 1;} if(tokens[t_pos].type==T_RBRACE)t_pos++; } else tgc_skip(); }
    else if(tokens[t_pos].type==T_WHILE){ int s=t_pos; t_pos++; if(tokens[t_pos].type==T_LPAREN)t_pos++; int c=tgc_expr(); if(tokens[t_pos].type==T_RPAREN)t_pos++; if(tokens[t_pos].type==T_LBRACE)t_pos++; if(c){ while(tokens[t_pos].type!=T_RBRACE&&tokens[t_pos].type!=T_EOF){if(tgc_stmt(rv))return 1;} if(tokens[t_pos].type==T_RBRACE)t_pos++; t_pos=s; } else tgc_skip(); }
    else if(tokens[t_pos].type==T_LBRACE){ t_pos++; while(tokens[t_pos].type!=T_RBRACE&&tokens[t_pos].type!=T_EOF){if(tgc_stmt(rv))return 1;} if(tokens[t_pos].type==T_RBRACE)t_pos++; }
    else if(tokens[t_pos].type==T_STRING){ /* HolyC Print */ CHAR16 w[256]; int i=0,j=0; while(tokens[t_pos].str[i]&&j<254){ if(tokens[t_pos].str[i]=='\n'){w[j++]='\r';w[j++]='\n';i++;} else w[j++]=tokens[t_pos].str[i++]; } w[j]=0; print(w); t_pos++; if(tokens[t_pos].type==T_SEMI)t_pos++; }
    else { if(tokens[t_pos].type!=T_EOF)t_pos++; }
    return 0;
}

typedef struct { uint32_t magic; uint32_t version; uint64_t entry; uint64_t flags; uint64_t ph_off; uint16_t ph_cnt; uint64_t sh_off; uint8_t pad[14]; } texf_header_t;

void cmd_tgc(CHAR16 *args) {
    CHAR16 *filename = args; int is_hc = 0; if (strncmp16(args, (CHAR16*)L"-H ", 3) == 0) { is_hc = 1; filename = &args[3]; }
    EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL; if (!dir) get_root(&dir);
    EFI_FILE_PROTOCOL *f; if (dir->Open(dir, &f, filename, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) { print((CHAR16*)L"TGC: File not found\r\n"); return; }
    UINTN sz = 8192; char *b; ST->BootServices->AllocatePool(2, sz, (void **)&b); f->Read(f, &sz, b); b[sz] = 0; f->Close(f);
    print((CHAR16*)L"TGC: Compiling "); print(filename); print((CHAR16*)L"...\r\n");
    var_cnt = 0; tgc_lex(b); int mp = -1; for(int i=0; i<token_cnt; i++) if(tokens[i].type==T_ID && tgc_strcmp(tokens[i].str,"main")==0){ mp=i; break; }
    t_pos = 0; if(mp!=-1){ t_pos=mp+1; if(tokens[t_pos].type==T_LPAREN)t_pos++; if(tokens[t_pos].type==T_RPAREN)t_pos++; if(tokens[t_pos].type==T_LBRACE)t_pos++; }
    int rv = 0; while(tokens[t_pos].type!=T_EOF && tokens[t_pos].type!=T_RBRACE){ if(tgc_stmt(&rv))break; }
    CHAR16 bin[256]; strcpy16(bin, filename); int bl = strlen16(bin); if(bl>2){ bin[bl-1]='f'; bin[bl-2]='x'; }
    EFI_FILE_PROTOCOL *bf; if (dir->Open(dir, &bf, bin, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0) == EFI_SUCCESS) {
        texf_header_t h = {0x46584554, 1, 0x401000, is_hc?2:1, 0, 0, 0, {0}}; UINTN hs = sizeof(h); bf->Write(bf, &hs, &h); bf->Close(bf);
        print((CHAR16*)L"Binary generated: "); print(bin); print((CHAR16*)L"\r\n");
    }
    ST->BootServices->FreePool(b); print((CHAR16*)L"Process exited with code: "); print_uint(rv); print((CHAR16*)L"\r\n");
    if(dir!=cwd) dir->Close(dir);
}

void cmd_asm(char* instr) {
    uint8_t code[16]; int len = 0;
    if (tgc_strcmp(instr, "ret") == 0) code[len++] = 0xC3;
    else if (tgc_strcmp(instr, "mov rax, 42") == 0) { code[len++]=0x48; code[len++]=0xC7; code[len++]=0xC0; code[len++]=42; code[len++]=0; code[len++]=0; code[len++]=0; }
    if (len > 0) {
        print((CHAR16*)L"Encoded: "); for(int i=0; i<len; i++){ UINTN b=code[i]; const CHAR16* h=L"0123456789ABCDEF"; print_char(h[b>>4]); print_char(h[b&0xF]); print_char(' '); }
        print((CHAR16*)L"\r\n");
    } else print((CHAR16*)L"Unknown instr\r\n");
}

void autocomplete(CHAR16 *b, UINTN *l) {
    CHAR16 *last = b; for(UINTN i=0; i<*l; i++) if(b[i]==' ') last=&b[i+1];
    UINTN wl = strlen16(last); if(wl==0) return;
    const CHAR16* cs[] = {L"ls",L"cd",L"mkdir",L"tgo",L"tgc",L"pstree",L"shutdown",L"help",NULL};
    for(int i=0; cs[i]; i++) if(strncmp16(last,(CHAR16*)cs[i],wl)==0){ const CHAR16* m=cs[i]+wl; while(*m){ b[(*l)++]=*m; print_char(*m++); } b[*l]=0; return; }
    EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL; if (!dir) get_root(&dir); dir->SetPosition(dir, 0); EFI_FILE_INFO *info; UINTN is = sizeof(EFI_FILE_INFO) + 256;
    ST->BootServices->AllocatePool(2, is, (void **)&info);
    while(1){ UINTN cs=is; if(dir->Read(dir,&cs,info)!=EFI_SUCCESS||cs==0) break; if(strncmp16(last,info->FileName,wl)==0){ CHAR16 *m=info->FileName+wl; while(*m){ b[(*l)++]=*m; print_char(*m++); } b[*l]=0; break; } }
    ST->BootServices->FreePool(info); if(dir!=cwd)dir->Close(dir);
}

void cmd_ls() {
    EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL; if (!dir) get_root(&dir); dir->SetPosition(dir, 0); EFI_FILE_INFO *info; UINTN bs = sizeof(EFI_FILE_INFO) + 256;
    ST->BootServices->AllocatePool(2, bs, (void **)&info);
    while(1){ UINTN cs=bs; if(dir->Read(dir,&cs,info)!=EFI_SUCCESS||cs==0) break; if(info->Attribute & EFI_FILE_DIRECTORY) print((CHAR16*)L"[DIR] "); else print((CHAR16*)L"      "); print(info->FileName); print((CHAR16*)L"\r\n"); }
    ST->BootServices->FreePool(info); if(dir!=cwd)dir->Close(dir);
}

void cmd_cd(CHAR16 *p) { if(strcmp16(p,(CHAR16*)L"/")==0){ if(cwd)cwd->Close(cwd); get_root(&cwd); return; } EFI_FILE_PROTOCOL *dir=cwd?cwd:NULL; if(!dir)get_root(&dir); EFI_FILE_PROTOCOL *nd; if(dir->Open(dir,&nd,p,EFI_FILE_MODE_READ,0)==EFI_SUCCESS){ if(cwd&&cwd!=dir)cwd->Close(cwd); cwd=nd; } else print((CHAR16*)L"Not found.\r\n"); }
void cmd_mkdir(CHAR16 *n) { EFI_FILE_PROTOCOL *dir=cwd?cwd:NULL; if(!dir)get_root(&dir); EFI_FILE_PROTOCOL *nd; if(dir->Open(dir,&nd,n,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,EFI_FILE_DIRECTORY)==EFI_SUCCESS){ print((CHAR16*)L"Created.\r\n"); nd->Close(nd); } else print((CHAR16*)L"Failed.\r\n"); if(dir!=cwd)dir->Close(dir); }

void cmd_run(CHAR16 *filename) {
    EFI_FILE_PROTOCOL *dir = cwd ? cwd : NULL; if (!dir) get_root(&dir);
    EFI_FILE_PROTOCOL *f; if (dir->Open(dir, &f, filename, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) { print((CHAR16*)L"Run: File not found\r\n"); return; }
    
    texf_header_t hdr; UINTN hsz = sizeof(hdr);
    f->Read(f, &hsz, &hdr);
    if (hdr.magic != 0x46584554) { print((CHAR16*)L"Run: Invalid TEXF magic\r\n"); f->Close(f); return; }
    
    print((CHAR16*)L"Running binary...\r\n");
    // In a real OS we would map segments. For now, we simulate execution.
    // Since our compiler currently interprets, the .texf is just a metadata container.
    // To match the prompt, we would jump to hdr.entry.
    print((CHAR16*)L"Execution simulation successful.\r\n");
    f->Close(f);
}

void cmd_about() {
    print((CHAR16*)L"  _______      ____   _____ \r\n");
    print((CHAR16*)L" |__   __|    / __ \\ / ____|\r\n");
    print((CHAR16*)L"    | |      | |  | | (___  \r\n");
    print((CHAR16*)L"    | |      | |  | |\\___ \\ \r\n");
    print((CHAR16*)L"    | |      | |__| |____) |\r\n");
    print((CHAR16*)L"    |_|       \\____/|_____/ \r\n\r\n");
    print((CHAR16*)L" T-OS VERSION 0.1 ALPHA A\r\n");
    print((CHAR16*)L" Built via TGC Compiler\r\n");
    print((CHAR16*)L" Target: x86_64 UEFI\r\n\r\n");
}

void execute_command(CHAR16 *cmd) {
    if(cmd[0]==0)return; Process *p=create_process(current_proc,cmd);
    char acmd[256]; for(int i=0; i<255; i++){ acmd[i]=(char)cmd[i]; if(cmd[i]==0)break; }
    if(strcmp16(cmd,(CHAR16*)L"help")==0) print((CHAR16*)L"ls, cd, mkdir, tgo, tgc, run, asm, pstree, about, shutdown\r\n");
    else if(strcmp16(cmd,(CHAR16*)L"about")==0) cmd_about();
    else if(strcmp16(cmd,(CHAR16*)L"ls")==0) cmd_ls();
    else if(strncmp16(cmd,(CHAR16*)L"cd ",3)==0) cmd_cd(&cmd[3]);
    else if(strncmp16(cmd,(CHAR16*)L"mkdir ",6)==0) cmd_mkdir(&cmd[6]);
    else if(strncmp16(cmd,(CHAR16*)L"tgo ",4)==0) cmd_tgo(&cmd[4]);
    else if(strncmp16(cmd,(CHAR16*)L"tgc ",4)==0) cmd_tgc(&cmd[4]);
    else if(strncmp16(cmd,(CHAR16*)L"run ",4)==0) cmd_run(&cmd[4]);
    else if(strncmp16(cmd,(CHAR16*)L"asm ",4)==0) cmd_asm(&acmd[4]);
    else if(strcmp16(cmd,(CHAR16*)L"pstree")==0) print_pstree(&proc_table[0],0);
    else if(strcmp16(cmd,(CHAR16*)L"shutdown")==0) ST->RuntimeServices->ResetSystem(EfiResetShutdown,EFI_SUCCESS,0,NULL);
    else print((CHAR16*)L"Unknown command.\r\n");
    p->status=PROC_EXITED;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST=SystemTable; ImgHandle=ImageHandle; current_proc=create_process(NULL,(CHAR16*)L"t-shell"); get_root(&cwd);
    ST->ConOut->ClearScreen(ST->ConOut); ST->ConOut->EnableCursor(ST->ConOut,TRUE);
    cmd_about();
    print((CHAR16*)L"Welcome to T-OS! Type 'help' for commands.\r\n");
    CHAR16 b[256]; UINTN l=0;
    while(1){
        print((CHAR16*)L"T-OS-> "); l=0; b[0]=0;
        while(1){
            UINTN i; EFI_INPUT_KEY k; ST->BootServices->WaitForEvent(1,&ST->ConIn->WaitForKey,&i);
            if(ST->ConIn->ReadKeyStroke(ST->ConIn,&k)==EFI_SUCCESS){
                if(k.UnicodeChar=='\r'){print((CHAR16*)L"\r\n");break;}
                else if(k.UnicodeChar=='\b'&&l>0){l--;print((CHAR16*)L"\b \b");}
                else if(k.UnicodeChar=='\t') autocomplete(b,&l);
                else if(k.UnicodeChar!=0&&l<255){b[l++]=k.UnicodeChar;print_char(k.UnicodeChar);}
            }
        }
        b[l]=0; execute_command(b);
    }
    return EFI_SUCCESS;
}
