#ifndef __OPENBMC_H__
#define __OPENBMC_H__

#include <stdint.h>
#include <stdio.h>

//select which dbus
#define DBUS_TYPE  G_BUS_TYPE_SYSTEM

// Macros
#define GET_VARIANT(v)         g_variant_get_variant(v) 
#define GET_VARIANT_D(v)       g_variant_get_double(g_variant_get_variant(v))
#define GET_VARIANT_U(v)       g_variant_get_uint32(g_variant_get_variant(v))
#define GET_VARIANT_B(v)       g_variant_get_byte(g_variant_get_variant(v))
#define NEW_VARIANT_D(v)       g_variant_new_variant(g_variant_new_double(v)) 
#define NEW_VARIANT_U(v)       g_variant_new_variant(g_variant_new_uint32(v)) 
#define NEW_VARIANT_B(v)       g_variant_new_variant(g_variant_new_byte(v)) 
#define VARIANT_COMPARE(x,y)   g_variant_compare(GET_VARIANT(x),GET_VARIANT(y))

#ifdef __arm__
static inline void devmem(void* addr, uint32_t val)
{
	printf("devmem %p = 0x%08x\n",addr,val);
        asm volatile("" : : : "memory");
        *(volatile uint32_t *)addr = val;
}
static inline uint32_t devmem_read(void* addr)
{
        asm volatile("" : : : "memory");
	return *(volatile uint32_t *)addr;
}
//static inline devmem(uint32_t reg, uint32_t val)
//{
//	printf("devmem 0x%08x = 0x%08x\n",reg,val);
//	//void* r = (void*)reg;
 //       write_reg(reg,val);
//}
#else
static inline uint32_t devmem(uint32_t val, uint32_t reg)
{
}
static inline uint32_t devmem_read(void* addr)
{
	return 0;
}

#endif

typedef struct {
	gint argc;
	gchar **argv;
	GMainLoop *loop;
	gpointer user_data;	

} cmdline;



#endif
