#ifndef __MAIN_H__
#define __MAIN_H__

#define MEM_BLOCK_SIZE 1024
#define MEM_BASE_ADDR (0x01C20000)

static volatile unsigned int *g_gpio_mem;

typedef struct __tagKeyState {
  char key0;
  char key1;
  char key2;
  char key3;
  char key4;
  char key5;
  char key6;
  char key7;
  char key8;
  char key9;
  char keyC;
  char keyA;
} KeyState;

#endif
