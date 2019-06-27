#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>

char *file_base;
int ei_class;

int parser_check_magic() {
    if (file_base[0] != 0x7f || file_base[1] != 'E'
     || file_base[2] != 'L'  || file_base[3] != 'F') {
        printf("Magic: %x %x %x %x [Invalid ELF Magic]\n", 
            file_base[0], file_base[1], file_base[2], file_base[3]);
        return -1;
     }

    printf("Magic: %x %x %x %x [Valid ELF Magic]\n", 
        file_base[0], file_base[1], file_base[2], file_base[3]);

    switch (file_base[4]) {
        case 0:
            printf("e_ident[EI_CLASS] == 0 (Invalid)\n");
            return -1;
        case ELFCLASS32:
            printf("e_ident[EI_CLASS] == 1 (ELFCLASS32)\n");
            break;
        case ELFCLASS64:
            printf("e_ident[EI_CLASS] == 2 (ELFCLASS64)\n");
            break;
        default:
            printf("e_ident[EI_CLASS] == %d (Invalid)\n", file_base[4]);
    }

    switch (file_base[5]) {
        case ELFDATANONE:
            printf("e_ident[EI_DATA] == 0 (Invalid)\n");
            return -1;
        case ELFDATA2LSB:
            printf("e_ident[EI_DATA] == 1 (ELFDATA2LSB, little endian)\n");
            break;
        case ELFDATA2MSB:
            printf("e_ident[EI_DATA] == 2 (ELFDATA2MSB, big endian)\n");
            break;
        default:
            printf("e_ident[EI_DATA] == %d (Invalid)\n", file_base[5]);
    }
    
    switch (file_base[6]) {
        case EV_CURRENT:
            printf("e_ident[EI_VERSION] == %d (EV_CURRENT)\n", EV_CURRENT);
            break;
        default:
            printf("e_ident[EI_VERSION] == %d (Invalid)\n", file_base[6]);
    }
    return 0;
}

int parser_init64() {
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) file_base;
    switch (hdr->e_type) {
        case ET_NONE:
            printf("e_type == %d (ET_NONE)\n", hdr->e_type);
            break;
        case ET_REL:
            printf("e_type == %d (ET_REL)\n", hdr->e_type);
            break;
        case ET_EXEC:
            printf("e_type == %d (ET_EXEC)\n", hdr->e_type);
            break;
        case ET_DYN:
            printf("e_type == %d (ET_DYN)\n", hdr->e_type);
            break;
        case ET_CORE:
            printf("e_type == %d (ET_CORE)\n", hdr->e_type);
            break;
        case ET_LOOS:
            printf("e_type == %d (ET_LOOS)\n", hdr->e_type);
            break;
        case ET_HIOS:
            printf("e_type == %d (ET_HIOS)\n", hdr->e_type);
            break;
        case ET_LOPROC:
            printf("e_type == %d (ET_LOPROC)\n", hdr->e_type);
            break;
        case ET_HIPROC:
            printf("e_type == %d (ET_HIPROC)\n", hdr->e_type);
            break;
        default:
            printf("e_type == %d (Unknown)\n", hdr->e_type);
            return -1;
    }

    switch (hdr->e_machine) {
        case EM_NONE:
            printf("e_machine == %d (EM_NONE)\n", hdr->e_machine);
            break;
        case EM_386:
            printf("e_machine == %d (EM_386)\n", hdr->e_machine);
            break;
        case EM_AARCH64:
            printf("e_machine == %d (EM_AARCH64)\n", hdr->e_machine);
            break;
        case EM_X86_64:
            printf("e_machine == %d (EM_X86_64)\n", hdr->e_machine);
            break;
        default:
            printf("e_machine == %d (Unknown)\n", hdr->e_machine);
            return -1;
    }

    switch (hdr->e_version) {
        case EV_NONE:
            printf("e_version == %d (EV_NONE)\n", hdr->e_version);
            return -1;
        case EV_CURRENT:
            printf("e_version == %d (EV_CURRENT)\n", hdr->e_version);
            break;
    }
}

int parser_init(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat");
        return -1;
    }
    int filesize = st.st_size;
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    file_base = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    
    if (parser_check_magic()) {
        printf("Invalid header, abort\n");
        return -1;
    }

    ei_class = file_base[4];
    
    if (ei_class == ELFCLASS32) {
        printf("No ELF32 support yet, abort\n");
        return -1;
    } else if (ei_class == ELFCLASS64) {
        return parser_init64();
    }
}

int main(int argc, char *argv[]) {
    printf("Simple ELF Parser\n");
    if (argc != 2) {
        printf("Usage: %s elf_filename\n", argv[0]);
        return -1;
    }
    parser_init(argv[1]);
} 