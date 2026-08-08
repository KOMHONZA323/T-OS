#!/bin/bash
# Locate the UEFI firmware images QEMU needs.
#
# Sourced by the run scripts. Sets OVMF_CODE_PATH and (when available)
# OVMF_VARS_PATH. Distributions disagree both on the directory and on the file
# name: Debian/Ubuntu ship OVMF_CODE_4M.fd these days, older releases and
# Fedora/Arch ship OVMF_CODE.fd.

find_ovmf() {
    OVMF_CODE_PATH=""
    OVMF_VARS_PATH=""

    local dirs=(
        /usr/share/OVMF
        /usr/share/ovmf
        /usr/share/ovmf/x64
        /usr/share/edk2-ovmf/x64
        /usr/share/edk2/ovmf
        /usr/share/edk2/x64
        /usr/share/qemu
    )
    # Most specific name first; the *_4M variants are the current Debian ones.
    local names=(OVMF_CODE_4M.fd OVMF_CODE.fd OVMF.fd edk2-x86_64-code.fd)

    local d n
    for d in "${dirs[@]}"; do
        for n in "${names[@]}"; do
            if [ -f "$d/$n" ]; then
                OVMF_CODE_PATH="$d/$n"
                # Pair the code image with the matching writable vars image.
                local vars="${n/CODE/VARS}"
                if [ -f "$d/$vars" ]; then
                    OVMF_VARS_PATH="$d/$vars"
                elif [ -f "$d/OVMF_VARS_4M.fd" ]; then
                    OVMF_VARS_PATH="$d/OVMF_VARS_4M.fd"
                elif [ -f "$d/OVMF_VARS.fd" ]; then
                    OVMF_VARS_PATH="$d/OVMF_VARS.fd"
                fi
                return 0
            fi
        done
    done
    return 1
}
