#include <uefi.h>

#include "kernel/bootparam.h"

/*** ELF64 defines and structs ***/
#define ELFMAG "\177ELF"
#define SELFMAG 4
#define EI_CLASS 4    /* File class byte index */
#define ELFCLASS64 2  /* 64-bit objects */
#define EI_DATA 5     /* Data encoding byte index */
#define ELFDATA2LSB 1 /* 2's complement, little endian */
#define ET_EXEC 2     /* Executable file */
#define PT_LOAD 1     /* Loadable program segment */
#define EM_MACH 62    /* AMD x86-64 architecture */

typedef struct
{
    uint8_t e_ident[16];  /* Magic number and other info */
    uint16_t e_type;      /* Object file type */
    uint16_t e_machine;   /* Architecture */
    uint32_t e_version;   /* Object file version */
    uint64_t e_entry;     /* Entry point virtual address */
    uint64_t e_phoff;     /* Program header table file offset */
    uint64_t e_shoff;     /* Section header table file offset */
    uint32_t e_flags;     /* Processor-specific flags */
    uint16_t e_ehsize;    /* ELF header size in bytes */
    uint16_t e_phentsize; /* Program header table entry size */
    uint16_t e_phnum;     /* Program header table entry count */
    uint16_t e_shentsize; /* Section header table entry size */
    uint16_t e_shnum;     /* Section header table entry count */
    uint16_t e_shstrndx;  /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
    uint32_t p_type;   /* Segment type */
    uint32_t p_flags;  /* Segment flags */
    uint64_t p_offset; /* Segment file offset */
    uint64_t p_vaddr;  /* Segment virtual address */
    uint64_t p_paddr;  /* Segment physical address */
    uint64_t p_filesz; /* Segment size in file */
    uint64_t p_memsz;  /* Segment size in memory */
    uint64_t p_align;  /* Segment alignment */
} Elf64_Phdr;

bootparam_t bootp;

int set_graphic_mode()
{
    efi_status_t status;
    efi_gop_t *gop = NULL;

    unsigned int width, height, pitch;
    unsigned char *lfb;

    efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = BS->LocateProtocol(&gopGuid, NULL, (void **)&gop);
    if (EFI_ERROR(status) && gop)
    {
        printf("unable to set video mode\n");
        return 1;
    }

    uint32_t mode_count = gop->Mode->MaxMode;
    for (uint32_t mode = 0; mode < mode_count; mode++)
    {
        uintn_t size_of_info;
        efi_gop_mode_info_t *info;
        status = gop->QueryMode(gop, mode, &size_of_info, &info);
        if (EFI_ERROR(status))
            continue;

        if (info->HorizontalResolution == 640 && info->VerticalResolution == 480)
        {
            status = gop->SetMode(gop, mode);
            if (!EFI_ERROR(status))
            {
                ST->ConOut->Reset(ST->ConOut, mode);
                ST->StdErr->Reset(ST->StdErr, mode);
                printf("Set VGA-compatible mode: %dx%d\n", info->HorizontalResolution, info->VerticalResolution);
                return 0;
            }
        }
    }

    printf("Error: Unable to set VGA-compatible mode\n");
    return 1;
}

char *read_file(const char *filename)
{
    FILE *f;
    long size;
    char *buff;

    f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "Unable to open file\n");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buff = malloc(size + 1);
    if (!buff)
    {
        fprintf(stderr, "unable to allocate memory\n");
        fclose(f);
        return NULL;
    }

    fread(buff, size, 1, f);
    fclose(f);

    return buff;
}

int set_bootp(int argc, char **argv)
{
    if (argc > 1)
    {
        bootp.argc = argc - 1;
        bootp.argv = (char **)malloc(argc * sizeof(char *));
        if (bootp.argv)
        {
            for (int i = 0; i < bootp.argc; i++)
                if ((bootp.argv[i] = (char *)malloc(strlen(argv[i + 1]) + 1)))
                    strcpy(bootp.argv[i], argv[i + 1]);
            bootp.argv[bootp.argc - 1] = NULL;
        }
    }

    efi_gop_t *gop = NULL;
    efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    efi_status_t status = BS->LocateProtocol(&gopGuid, NULL, (void **)&gop);
    if (EFI_ERROR(status) && gop)
    {
        printf("unable to set video mode\n");
        return 1;
    }

    bootp.framebuffer = (unsigned int *)gop->Mode->FrameBufferBase;
    bootp.width = gop->Mode->Information->HorizontalResolution;
    bootp.height = gop->Mode->Information->VerticalResolution;
    bootp.pitch = sizeof(unsigned int) * gop->Mode->Information->PixelsPerScanLine;

    return 0;
}

uintptr_t is_valid_elf(char *buff, Elf64_Ehdr *elf)
{

    if (!(!memcmp(elf->e_ident, ELFMAG, SELFMAG) && /* magic match? */
          elf->e_ident[EI_CLASS] == ELFCLASS64 &&   /* 64 bit? */
          elf->e_ident[EI_DATA] == ELFDATA2LSB &&   /* LSB? */
          elf->e_type == ET_EXEC &&                 /* executable object? */
          elf->e_machine == EM_MACH &&              /* architecture match? */
          elf->e_phnum > 0))
    {
        fprintf(stderr, "not a valid ELF executable for this architecture\n");
        return 0;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)(buff + elf->e_phoff);
    for (int i = 0; i < elf->e_phnum; i++)
    {

        if (phdr->p_type == PT_LOAD)
        {
            printf("ELF segment %p %d bytes (bss %d bytes)\n", phdr->p_vaddr, phdr->p_filesz,
                   phdr->p_memsz - phdr->p_filesz);
            memcpy((void *)phdr->p_vaddr, buff + phdr->p_offset, phdr->p_filesz);
            memset((void *)(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
        }
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + elf->e_phentsize);
    }

    return elf->e_entry;
}

int main(int argc, char **argv)
{
    if (set_graphic_mode())
        return 1;
    memset(&bootp, 0, sizeof(bootparam_t));
    if (set_bootp(argc, argv))
        return 1;

    char *buff = read_file("\\EFI\\BOOT\\kernel.elf");
    uintptr_t entry = is_valid_elf(buff, (Elf64_Ehdr *)buff);
    if (entry == 0)
        return 1;
    free(buff);

    printf("ELF entry point %p\n", entry);
    int returned_value = (*((int (*__attribute__((sysv_abi)))(bootparam_t *))(entry)))(&bootp);
    printf("ELF returned %d\n", returned_value);

    while (1)
    {
    }

    return 0;
}
