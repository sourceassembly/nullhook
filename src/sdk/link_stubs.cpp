#include "common.hpp"

extern "C" bool __sync_bool_compare_and_swap_16(volatile void *ptr, unsigned __int128 oldv, unsigned __int128 newv)
{
    unsigned long long old_lo = static_cast<unsigned long long>(oldv);
    unsigned long long old_hi = static_cast<unsigned long long>(oldv >> 64);
    unsigned long long new_lo = static_cast<unsigned long long>(newv);
    unsigned long long new_hi = static_cast<unsigned long long>(newv >> 64);
    unsigned char ok;
    __asm__ __volatile__("lock cmpxchg16b %0"
                         : "+m"(*reinterpret_cast<unsigned __int128 *>(const_cast<void *>(ptr))), "+a"(old_lo), "+d"(old_hi), "=@ccz"(ok)
                         : "b"(new_lo), "c"(new_hi)
                         : "memory");
    return ok;
}

#include <bitbuf.h>
#include <characterset.h>
#include <datacache/imdlcache.h>
#include <mathlib/vmatrix.h>
#include <studio.h>
#include <tier1/strtools.h>

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <unistd.h>

void CharacterSetBuild(characterset_t *pSetBuffer, const char *pSetString)
{
    std::memset(pSetBuffer, 0, sizeof(*pSetBuffer));
    if (!pSetString)
        return;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(pSetString); *p; ++p)
        pSetBuffer->set[*p] = 1;
}

int V_stricmp(const char *s1, const char *s2)
{
    return strcasecmp(s1 ? s1 : "", s2 ? s2 : "");
}

int V_strncmp(const char *s1, const char *s2, int count)
{
    return std::strncmp(s1 ? s1 : "", s2 ? s2 : "", static_cast<size_t>(count < 0 ? 0 : count));
}

char *V_strupr(char *start)
{
    if (!start)
        return start;
    for (char *p = start; *p; ++p)
        *p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
    return start;
}

char *V_strlower(char *start)
{
    if (!start)
        return start;
    for (char *p = start; *p; ++p)
        *p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
    return start;
}

void V_strncpy(char *pDest, const char *pSrc, int maxLenInChars)
{
    if (!pDest || maxLenInChars <= 0)
        return;
    if (!pSrc)
        pSrc = "";
    std::snprintf(pDest, static_cast<size_t>(maxLenInChars), "%s", pSrc);
}

int V_vsnprintf(char *pDest, int maxLenInCharacters, const char *pFormat, va_list params)
{
    if (!pDest || maxLenInCharacters <= 0)
        return 0;
    return std::vsnprintf(pDest, static_cast<size_t>(maxLenInCharacters), pFormat ? pFormat : "", params);
}

int V_snprintf(char *pDest, int maxLenInChars, const char *pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    const int n = V_vsnprintf(pDest, maxLenInChars, pFormat, args);
    va_end(args);
    return n;
}

char *V_strncat(char *pDest, const char *pSrc, size_t cchDest, int max_chars_to_copy)
{
    if (!pDest || !pSrc || cchDest == 0)
        return pDest;
    const size_t used = std::strlen(pDest);
    if (used + 1 >= cchDest)
        return pDest;
    size_t remain = cchDest - used - 1;
    if (max_chars_to_copy >= 0 && static_cast<size_t>(max_chars_to_copy) < remain)
        remain = static_cast<size_t>(max_chars_to_copy);
    std::strncat(pDest, pSrc, remain);
    pDest[cchDest - 1] = '\0';
    return pDest;
}

const char *V_strnchr(const char *pStr, char c, int n)
{
    if (!pStr || n <= 0)
        return nullptr;
    for (int i = 0; i < n && pStr[i]; ++i)
    {
        if (pStr[i] == c)
            return pStr + i;
    }
    return nullptr;
}

const char *V_stristr(const char *pStr, const char *pSearch)
{
    if (!pStr || !pSearch || !*pSearch)
        return pStr;
    const size_t n = std::strlen(pSearch);
    for (const char *p = pStr; *p; ++p)
    {
        if (strncasecmp(p, pSearch, n) == 0)
            return p;
    }
    return nullptr;
}

char *V_stristr(char *pStr, const char *pSearch)
{
    return const_cast<char *>(V_stristr(static_cast<const char *>(pStr), pSearch));
}

const char *V_strnistr(const char *pStr, const char *pSearch, int n)
{
    if (!pStr || !pSearch || !*pSearch || n <= 0)
        return nullptr;
    const int search_len = static_cast<int>(std::strlen(pSearch));
    for (int i = 0; i <= n - search_len && pStr[i]; ++i)
    {
        if (strncasecmp(pStr + i, pSearch, static_cast<size_t>(search_len)) == 0)
            return pStr + i;
    }
    return nullptr;
}

int64 V_atoi64(const char *str)
{
    return str ? static_cast<int64>(std::strtoll(str, nullptr, 10)) : 0;
}

void V_FixSlashes(char *pname, char separator)
{
    if (!pname)
        return;
    for (; *pname; ++pname)
    {
        if (*pname == '/' || *pname == '\\')
            *pname = separator;
    }
}

void V_StripTrailingSlash(char *ppath)
{
    if (!ppath || !*ppath)
        return;
    const size_t n = std::strlen(ppath);
    if (n && (ppath[n - 1] == '/' || ppath[n - 1] == '\\'))
        ppath[n - 1] = '\0';
}

const char *V_UnqualifiedFileName(const char *path)
{
    if (!path)
        return "";
    const char *slash = std::strrchr(path, '/');
    const char *bslash = std::strrchr(path, '\\');
    if (bslash && (!slash || bslash > slash))
        slash = bslash;
    return slash ? slash + 1 : path;
}

void V_FileBase(const char *in, char *out, int maxlen)
{
    if (!out || maxlen <= 0)
        return;
    const char *name = V_UnqualifiedFileName(in);
    V_strncpy(out, name, maxlen);
    char *dot = std::strrchr(out, '.');
    if (dot)
        *dot = '\0';
}

void V_StripExtension(const char *in, char *out, int outLen)
{
    V_strncpy(out, in ? in : "", outLen);
    char *dot = std::strrchr(out, '.');
    const char *slash = std::strrchr(out, '/');
    const char *bslash = std::strrchr(out, '\\');
    if (bslash && (!slash || bslash > slash))
        slash = bslash;
    if (dot && (!slash || dot > slash))
        *dot = '\0';
}

void V_ExtractFileExtension(const char *path, char *dest, int destSize)
{
    if (!dest || destSize <= 0)
        return;
    dest[0] = '\0';
    const char *name = V_UnqualifiedFileName(path);
    const char *dot  = std::strrchr(name, '.');
    if (dot && dot[1])
        V_strncpy(dest, dot + 1, destSize);
}

bool V_ExtractFilePath(const char *path, char *dest, int destSize)
{
    if (!dest || destSize <= 0)
        return false;
    dest[0] = '\0';
    if (!path)
        return false;
    const char *name = V_UnqualifiedFileName(path);
    const int n      = static_cast<int>(name - path);
    if (n <= 0)
        return false;
    const int copy = n < destSize ? n : destSize - 1;
    std::memcpy(dest, path, static_cast<size_t>(copy));
    dest[copy] = '\0';
    return true;
}

bool V_StripLastDir(char *dirName, int maxlen)
{
    (void) maxlen;
    if (!dirName || !*dirName)
        return false;
    V_StripTrailingSlash(dirName);
    char *slash = std::strrchr(dirName, '/');
    char *bslash = std::strrchr(dirName, '\\');
    if (bslash && (!slash || bslash > slash))
        slash = bslash;
    if (!slash)
        return false;
    slash[1] = '\0';
    return true;
}

void V_ComposeFileName(const char *path, const char *filename, char *dest, int destSize)
{
    if (!dest || destSize <= 0)
        return;
    V_strncpy(dest, path ? path : "", destSize);
    V_StripTrailingSlash(dest);
    if (filename && *filename)
    {
        V_strncat(dest, "/", static_cast<size_t>(destSize), COPY_ALL_CHARACTERS);
        V_strncat(dest, filename, static_cast<size_t>(destSize), COPY_ALL_CHARACTERS);
    }
}

bool V_RemoveDotSlashes(char *pFilename, char separator, bool bRemoveDoubleSlashes)
{
    if (!pFilename)
        return false;
    V_FixSlashes(pFilename, separator);
    if (bRemoveDoubleSlashes)
    {
        char *in = pFilename;
        char *out = pFilename;
        while (*in)
        {
            *out++ = *in;
            if (*in == separator)
            {
                while (in[1] == separator)
                    ++in;
            }
            ++in;
        }
        *out = '\0';
    }
    return true;
}

void V_MakeAbsolutePath(char *pOut, int outLen, const char *pPath, const char *pStartingDir)
{
    if (!pOut || outLen <= 0)
        return;
    if (pPath && pPath[0] == '/')
    {
        V_strncpy(pOut, pPath, outLen);
        return;
    }
    char cwd[4096];
    const char *base = pStartingDir;
    if (!base || !*base)
    {
        if (!getcwd(cwd, sizeof(cwd)))
            cwd[0] = '\0';
        base = cwd;
    }
    V_ComposeFileName(base, pPath ? pPath : "", pOut, outLen);
}

void V_binarytohex(const unsigned char *in, int inputbytes, char *out, int outsize)
{
    if (!out || outsize <= 0)
        return;
    out[0] = '\0';
    if (!in || inputbytes <= 0)
        return;
    int used = 0;
    for (int i = 0; i < inputbytes && used + 2 < outsize; ++i)
        used += std::snprintf(out + used, static_cast<size_t>(outsize - used), "%02x", in[i]);
}

int Q_UTF8ToUTF32(const char *pUTF8, wchar_t *pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy)
{
    if (!pUTF32 || cubDestSizeInBytes < static_cast<int>(sizeof(wchar_t)))
        return 0;
    const int max_chars = cubDestSizeInBytes / static_cast<int>(sizeof(wchar_t));
    int n               = 0;
    if (pUTF8)
    {
        while (pUTF8[n] && n + 1 < max_chars)
        {
            pUTF32[n] = static_cast<wchar_t>(static_cast<unsigned char>(pUTF8[n]));
            ++n;
        }
    }
    pUTF32[n] = 0;
    return (n + 1) * static_cast<int>(sizeof(wchar_t));
}

int Q_UTF32ToUTF8(const wchar_t *pUTF32, char *pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy)
{
    if (!pUTF8 || cubDestSizeInBytes <= 0)
        return 0;
    int n = 0;
    if (pUTF32)
    {
        while (pUTF32[n] && n + 1 < cubDestSizeInBytes)
        {
            pUTF8[n] = static_cast<char>(pUTF32[n] & 0x7f);
            ++n;
        }
    }
    pUTF8[n] = '\0';
    return n + 1;
}

unsigned int HashStringCaseless(const char *s)
{
    unsigned int h = 2166136261u;
    if (!s)
        return h;
    for (; *s; ++s)
    {
        h ^= static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(*s)));
        h *= 16777619u;
    }
    return h;
}

unsigned int HashInt(int n)
{
    unsigned int x = static_cast<unsigned int>(n);
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

void bf_read::SetOverflowFlag()
{
    m_bOverflow = true;
}

int bf_read::ReadSBitLong(int numbits)
{
    const unsigned int r = ReadUBitLong(numbits);
    const unsigned int s = 1u << (numbits - 1);
    if (r >= s)
        return static_cast<int>(r - (s << 1));
    return static_cast<int>(r);
}

void MatrixCopy(const matrix3x4_t &in, matrix3x4_t &out)
{
    std::memcpy(&out, &in, sizeof(matrix3x4_t));
}

VMatrix &VMatrix::operator=(const VMatrix &other)
{
    if (this != &other)
        std::memcpy(m, other.m, sizeof(m));
    return *this;
}

void SetIdentityMatrix(matrix3x4_t &matrix)
{
    std::memset(&matrix, 0, sizeof(matrix));
    matrix[0][0] = matrix[1][1] = matrix[2][2] = 1.f;
}

void AngleIMatrix(const QAngle &angles, const Vector &position, matrix3x4_t &matrix)
{
    AngleMatrix(angles, position, matrix);
    matrix3x4_t tmp = matrix;
    MatrixInvert(tmp, matrix);
}

void VectorIRotate(const float *in1, const matrix3x4_t &in2, float *out)
{
    out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
    out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
    out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}

void VectorITransform(const float *in1, const matrix3x4_t &in2, float *out)
{
    const Vector t(in1[0] - in2[0][3], in1[1] - in2[1][3], in1[2] - in2[2][3]);
    VectorIRotate(t.Base(), in2, out);
}

void AngleQuaternion(const QAngle &angles, Quaternion &outQuat)
{
    AngleQuaternion(RadianEuler(DEG2RAD(angles.z), DEG2RAD(angles.x), DEG2RAD(angles.y)), outQuat);
}

float QuaternionDotProduct(const Quaternion &p, const Quaternion &q)
{
    return p.x * q.x + p.y * q.y + p.z * q.z + p.w * q.w;
}

void AxisAngleQuaternion(const Vector &axis, float angle, Quaternion &out)
{
    const float ha = DEG2RAD(angle) * 0.5f;
    float s, c;
    SinCos(ha, &s, &c);
    Vector n = axis;
    VectorNormalize(n);
    out.x = n.x * s;
    out.y = n.y * s;
    out.z = n.z * s;
    out.w = c;
}

void QuaternionAxisAngle(const Quaternion &q, Vector &axis, float &angle)
{
    angle = RAD2DEG(2.f * acosf(std::clamp(q.w, -1.f, 1.f)));
    const float s = sqrtf(std::max(1.f - q.w * q.w, 0.f));
    if (s > 0.0001f)
        axis = Vector(q.x / s, q.y / s, q.z / s);
    else
        axis = Vector(1.f, 0.f, 0.f);
}

void VectorYawRotate(const Vector &in, float flYaw, Vector &out)
{
    float s, c;
    SinCos(DEG2RAD(flYaw), &s, &c);
    out.x = in.x * c - in.y * s;
    out.y = in.x * s + in.y * c;
    out.z = in.z;
}

void Hermite_Spline(const Vector &p0, const Vector &p1, const Vector &p2, float t, Vector &out)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    out            = p1 * (2.f * t3 - 3.f * t2 + 1.f) + (p2 - p0) * (t3 - 2.f * t2 + t) * 0.5f + (p2 - p0) * (t3 - t2) * 0.5f + p1 * (-2.f * t3 + 3.f * t2);
    (void) p0;
}

void Hermite_Spline(const Quaternion &p0, const Quaternion &p1, const Quaternion &p2, float t, Quaternion &out)
{
    Vector v0(p0.x, p0.y, p0.z), v1(p1.x, p1.y, p1.z), v2(p2.x, p2.y, p2.z), vr;
    Hermite_Spline(v0, v1, v2, t, vr);
    out.x = vr.x;
    out.y = vr.y;
    out.z = vr.z;
    out.w = p1.w + (p2.w - p0.w) * t;
    QuaternionNormalize(out);
}

bool SolveQuadratic(float a, float b, float c, float &root1, float &root2)
{
    const float d = b * b - 4.f * a * c;
    if (d < 0.f)
        return false;
    if (a == 0.f)
    {
        if (b == 0.f)
            return false;
        root1 = root2 = -c / b;
        return true;
    }
    const float s = sqrtf(d);
    root1         = (-b + s) / (2.f * a);
    root2         = (-b - s) / (2.f * a);
    return true;
}

void RotationDeltaAxisAngle(const QAngle &src, const QAngle &dest, Vector &deltaAxis, float &deltaAngle)
{
    Quaternion q0, q1, qd;
    AngleQuaternion(src, q0);
    AngleQuaternion(dest, q1);
    qd.x        = q1.x - q0.x;
    qd.y        = q1.y - q0.y;
    qd.z        = q1.z - q0.z;
    qd.w        = q1.w - q0.w;
    QuaternionNormalize(qd);
    QuaternionAxisAngle(qd, deltaAxis, deltaAngle);
}

bool IntersectRayWithBox(const Vector &, const Vector &, const Vector &, const Vector &, float, CBaseTrace *, float *)
{
    return false;
}

bool IntersectRayWithOBB(const Ray_t &, const matrix3x4_t &, const Vector &, const Vector &, float, CBaseTrace *)
{
    return false;
}

virtualmodel_t *studiohdr_t::GetVirtualModel() const
{
    if (numincludemodels == 0)
        return nullptr;
    if (!g_IMDLCache)
        return static_cast<virtualmodel_t *>(VirtualModel());
    return g_IMDLCache->GetVirtualModelFast(this, static_cast<MDLHandle_t>(reinterpret_cast<uintptr_t>(VirtualModel()) & 0xffff));
}

byte *studiohdr_t::GetAnimBlock(int i) const
{
    if (!g_IMDLCache)
        return nullptr;
    return g_IMDLCache->GetAnimBlock(static_cast<MDLHandle_t>(reinterpret_cast<uintptr_t>(VirtualModel()) & 0xffff), i);
}
