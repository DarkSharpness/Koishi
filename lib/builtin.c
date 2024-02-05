typedef _Bool bool;
typedef unsigned size_t;

#define __MX_BUFFER_SIZE__ 256

// Necessary functions in libc

int printf(const char *pattern, ...);
int sprintf(char *dest, const char *pattern, ...);
int scanf(const char *pattern, ...);
int sscanf(const char *src, const char *pattern, ...);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *malloc(size_t n);
char *strcat(char *dest, const char *src);

char *__String_substring__(char *str,int l,int r) {
    int len = r - l;
    char *buf = malloc(len + 1);
    buf[len] = '\0';
    return memcpy(buf, str + l, len);
}

// Inlined
int __String_parseInt__(char *str) {
    int ret;
    sscanf(str, "%d", &ret);
    return ret;
}

// Inlined
int __String_ord__(char *str,int n) {
    return str[n];
}

void __print__(char *str) {
    printf("%s", str);
}

void __printInt__(int n) {
    printf("%d", n);
}

void __printlnInt__(int n) {
    printf("%d\n", n);
}

char *__getString__() {
    char *buf = malloc(__MX_BUFFER_SIZE__);
    scanf("%s", buf);
    return buf;
}

// Inlined
int __getInt__() {
    int ret;
    scanf("%d", &ret);
    return ret;
}

char *__toString__(int n) {
    /* Align to 16 */
    char *buf = malloc(16);
    sprintf(buf, "%d", n);
    return buf;
}

char *__string_add__(char *lhs,char *rhs) {
    int len1 = strlen(lhs);
    int len2 = strlen(rhs) + 1;
    char *buf = malloc(len1 + len2);
    char *tmp = memcpy(buf + len1, rhs, len2) - len1;
    return (char *)memcpy(tmp, lhs, len1);
}

// Inlined
int __Array_size__(void *array) {
    return *(((int *)array) - 1);
}

// Inlined
void *__new_array4__(int len) {
    int *buf = (int *)malloc(len * 4 + 4);
    *buf = len;
    return buf + 1;
}

// Inlined
void *__new_array1__(int len) {
    int *buf = (int *)malloc(len + 4);
    *buf = len;
    return buf + 1;
}