#define DEBUG_PRINT_HEX(s) printf(#s"=0x%X\n",s);
#define DEBUG_PRINT_HEX_LONG(s) printf(#s"=0x%lX\n",s);
#define DEBUG_PRINT_DEC(s) printf(#s"=%d\n",s);
#define DEBUG_PRINT_STRING(s) printf(#s"=%s\n",s);

#define MAGIC_BREAKPOINT(a,b) \
if(a == b) { \
 printf("MAGIC BREAKPOINT FOR CONDITION " #a " == " #b "\n",a,b); \
 asm("xchg %bx,%bx"); \
}


