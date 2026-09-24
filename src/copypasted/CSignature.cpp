#include "common.hpp"

#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

Elf64_Shdr *getSectionHeader(void *module, const char *sectionName)
{
    auto *ehdr = reinterpret_cast<Elf64_Ehdr *>(module);
    auto *shdr = reinterpret_cast<Elf64_Shdr *>(uintptr_t(module) + ehdr->e_shoff);
    Elf64_Shdr *strhdr = &shdr[ehdr->e_shstrndx];

    char *strtab        = nullptr;
    size_t strtabSize   = 0;
    if (strhdr != nullptr && strhdr->sh_type == SHT_STRTAB)
    {
        strtab     = reinterpret_cast<char *>(uintptr_t(module) + strhdr->sh_offset);
        strtabSize = strhdr->sh_size;
    }
    else
    {
        logging::Info("String table header was corrupted!");
        return nullptr;
    }

    for (int i = 0; i < ehdr->e_shnum; i++)
    {
        Elf64_Shdr *hdr = &shdr[i];
        if (hdr && hdr->sh_name < strtabSize)
        {
            if (!strcmp(strtab + hdr->sh_name, sectionName))
                return hdr;
        }
    }
    return nullptr;
}

bool InRange(char x, char a, char b)
{
    return x >= a && x <= b;
}

int GetBits(char x)
{
    if (InRange((char) (x & (~0x20)), 'A', 'F'))
        return (x & (~0x20)) - 'A' + 0xa;
    if (InRange(x, '0', '9'))
        return x - '0';
    return 0;
}
int GetBytes(const char *x)
{
    return GetBits(x[0]) << 4 | GetBits(x[1]);
}

uintptr_t CSignature::dwFindPattern(uintptr_t dwAddress, uintptr_t dwLength, const char *szPattern)
{
    const char *pattern  = szPattern;
    uintptr_t firstMatch = 0;

    uintptr_t start = dwAddress;
    uintptr_t end   = dwLength;

    for (uintptr_t pos = start; pos < end; pos++)
    {
        if (*pattern == 0)
            return firstMatch;

        const uint8_t currentPattern = *reinterpret_cast<const uint8_t *>(pattern);
        const uint8_t currentMemory  = *reinterpret_cast<const uint8_t *>(pos);

        if (currentPattern == '\?' || currentMemory == GetBytes(pattern))
        {
            if (firstMatch == 0)
                firstMatch = pos;

            const char *next = pattern + (currentPattern == '\?' ? 1 : 2);
            if (*next == ' ')
                ++next;
            if (*next == 0)
            {
                logging::Info("Found pattern \"%s\" at %p.", szPattern, reinterpret_cast<void *>(firstMatch));
                return firstMatch;
            }
            pattern = next;
        }
        else
        {
            pattern    = szPattern;
            firstMatch = 0;
        }
    }
    logging::Info("THIS IS SERIOUS: Could not locate signature: "
                  "\n============\n\"%s\"\n============",
                  szPattern);
    return 0;
}

void *CSignature::GetModuleHandleSafe(const char *pszModuleName)
{
    void *moduleHandle = nullptr;
    do
    {
        moduleHandle = dlopen(pszModuleName, RTLD_NOW);
        usleep(1);
    } while (moduleHandle == nullptr);
    return moduleHandle;
}

static CSignature_space::SharedObjStorage objects[CSignature_space::entry_count];

uintptr_t CSignature::GetSignature(const char *chPattern, sharedobj::SharedObject &obj, int idx)
{
    auto &object = objects[idx];
    if (!object.inited)
    {
        int fd       = open(obj.path.c_str(), O_RDONLY);
        if (fd < 0)
            return 0;
        off_t size = lseek(fd, 0, SEEK_END);
        void *module = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        if (module == MAP_FAILED)
            return 0;
        link_map *moduleMap = obj.lmap;

        Elf64_Shdr *textHeader = getSectionHeader(module, ".text");
        if (!textHeader)
            return 0;

        int textOffset = int(textHeader->sh_offset);
        int textSize   = int(textHeader->sh_size);

        object        = CSignature_space::SharedObjStorage(module, moduleMap, textOffset, textSize);
        object.inited = true;
    }

    uintptr_t patr = dwFindPattern(uintptr_t(object.module) + object.textOffset, uintptr_t(object.module) + object.textOffset + object.textSize, chPattern);
    if (!patr)
        return 0;
    return patr - uintptr_t(object.module) + object.moduleMap->l_addr;
}

uintptr_t CSignature::GetClientSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::client(), CSignature_space::client);
}
uintptr_t CSignature::GetEngineSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::engine(), CSignature_space::engine);
}
uintptr_t CSignature::GetLauncherSignature(const char *chaPattern)
{
    return GetSignature(chaPattern, sharedobj::launcher(), CSignature_space::launcher);
}
uintptr_t CSignature::GetSteamAPISignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::steamapi(), CSignature_space::steamapi);
}
uintptr_t CSignature::GetVstdSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::vstdlib(), CSignature_space::vstd);
}
uintptr_t CSignature::GetServerSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::server(), CSignature_space::server);
}
uintptr_t CSignature::GetMaterialSystemSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::materialsystem(), CSignature_space::materialsystem);
}
uintptr_t CSignature::GetSteamClientSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::steamclient(), CSignature_space::steamclient);
}

CSignature gSignatures;
