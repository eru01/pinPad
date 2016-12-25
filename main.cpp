#include "main.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>

using std::vector;

enum STATES {
  S_INITIAL,
  S_SET_PIN,
  S_ARMED,
  S_INPUT_PIN
};
enum STATES g_state;

static inline void clearConsole()
{
  printf("\r                                                       \r");
  fflush(stdout);
}


template <typename T>
static inline bool isContain(vector<T> &container, const T value)
{
  return std::find(container.begin(), container.end(), value) != container.end();
}


// GPIOピンをボタンモジュール用に設定
int initGPIO()
{
  // get register address
  if (!g_gpio_mem)
    {
      int fd = open("/dev/mem", O_RDWR|O_SYNC);
      if (fd == -1)
	{
	  puts("error: cannot open /dev/mem");
	  return 1;
	}

      g_gpio_mem = (unsigned int*)mmap(NULL, MEM_BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, MEM_BASE_ADDR);
      if ((int)g_gpio_mem == -1)
	{
	  puts("error: mmap error");
	  return 2;
	}
      close(fd);
    }

  // configulation
  // OUTPUT: PA0, PA1, PA2, PA3, PA6
  // INPUT:  PA7
  int pins[] = {0, 1, 2, 3, 6};
  unsigned int reg;
  for (int i=0; i<sizeof(pins)/sizeof(*pins); ++i)
    {
      reg = g_gpio_mem[0x200 + pins[i] / 8];
      reg = (reg & ~(0xF << (4 * (pins[i] % 8)))) | (1 << (4 * (pins[i] % 8)));
      g_gpio_mem[0x200 + pins[i] / 8] = reg;
    }
  reg = g_gpio_mem[0x200];
  reg = reg & ~(0xF << (4 * 7));
  g_gpio_mem[0x200] = reg;


  // set CLR_ to H
  reg = g_gpio_mem[0x200+0x4];
  reg = (reg & ~(1 << 2)) | (1 << 2);
  g_gpio_mem[0x200+0x4] = reg;

  return 0;
}

KeyState getKeys()
{
  if (!g_gpio_mem)
    initGPIO();

  // scan
  unsigned int reg;
  char btns[16] = {0,};
  for (int i=0; i<16; ++i)
    {
      // set MUX
      reg = g_gpio_mem[0x200+0x4];
      reg = (reg & ~(1 << 6)) | ((i & 1) << 6);
      reg = (reg & ~(1 << 1)) | (((i & 2) ? 1 : 0) << 1);
      reg = (reg & ~(1 << 0)) | (((i & 4) ? 1 : 0) << 0);
      reg = (reg & ~(1 << 3)) | (((i & 8) ? 1 : 0) << 3);
      g_gpio_mem[0x200+0x4] = reg;
      __asm("nop");

      // read data register
      reg = g_gpio_mem[0x200+0x4];
      btns[i] = reg & (1 << 7);
    }

  // reset D-FF
  reg = g_gpio_mem[0x200+0x4];
  reg = reg & ~(1 << 2);
  g_gpio_mem[0x200+0x4] = reg;
  __asm("nop");
  __asm("nop");
  reg = (reg & ~(1 << 2)) | (1 << 2);
  g_gpio_mem[0x200+0x4] = reg;

  // set return value
  KeyState ret = {0,};
  ret.key0 = btns[0] ? 1 : 0;
  ret.key1 = btns[1] ? 1 : 0;
  ret.key2 = btns[2] ? 1 : 0;
  ret.key3 = btns[3] ? 1 : 0;
  ret.key4 = btns[4] ? 1 : 0;
  ret.key5 = btns[5] ? 1 : 0;
  ret.key6 = btns[6] ? 1 : 0;
  ret.key7 = btns[7] ? 1 : 0;
  ret.key8 = btns[8] ? 1 : 0;
  ret.key9 = btns[9] ? 1 : 0;
  ret.keyC = btns[12] ? 1 : 0;
  ret.keyA = btns[13] ? 1 : 0;

  return ret;
}

int main(int argc, char *argv[])
{
  KeyState prev, cur;
  vector<int> pinCode;
  
  initGPIO();

  prev = getKeys();
  g_state = S_INITIAL; // ステートマシン初期化
  while (1)
    {
      cur = getKeys();

      if (0)
	{
	  printf("%d %d %d %d  %d %d %d  %d %d %d  %d %d\n", cur.key0, cur.key1, cur.key2, cur.key3, cur.key4, cur.key5, cur.key6, cur.key7, cur.key8, cur.key9, cur.keyC, cur.keyA);
	  usleep(300000);
	}

      // check buttons
      vector<int> btnPushed; // 押されたボタン
      if (cur.key0 && !prev.key0) btnPushed.push_back(0);
      if (cur.key1 && !prev.key1) btnPushed.push_back(1);
      if (cur.key2 && !prev.key2) btnPushed.push_back(2);
      if (cur.key3 && !prev.key3) btnPushed.push_back(3);
      if (cur.key4 && !prev.key4) btnPushed.push_back(4);
      if (cur.key5 && !prev.key5) btnPushed.push_back(5);
      if (cur.key6 && !prev.key6) btnPushed.push_back(6);
      if (cur.key7 && !prev.key7) btnPushed.push_back(7);
      if (cur.key8 && !prev.key8) btnPushed.push_back(8);
      if (cur.key9 && !prev.key9) btnPushed.push_back(9);
      if (cur.keyC && !prev.keyC) btnPushed.push_back(100);
      if (cur.keyA && !prev.keyA) btnPushed.push_back(200);

      if (0)
	if (btnPushed.size() > 0)
	  {
	    printf("pushed: ");
	    vector<int>::const_iterator it;
	    for (it=btnPushed.begin(); it!=btnPushed.end(); ++it)
	      printf("%d ", *it);
	    putchar('\n');
	  }

      // state machine
      switch (g_state)
	{
	  static time_t lastTick;
	  static const int maxTrailingDot = 3;
	  int trailingDot;
	  int verifyPos;
	  
	case S_INITIAL: // 初期状態
	  trailingDot = time(NULL) - lastTick;
	  if (trailingDot > maxTrailingDot)
	    {
	      trailingDot = 0;
	      lastTick = time(NULL);
	    }
	  clearConsole();
	  printf("WAITING");
	  for (int i=0; i<trailingDot; ++i) putchar('.'); fflush(stdout);

	  {
	    FILE *fp = fopen("/sys/class/leds/red_led/brightness", "w");
	    fprintf(fp, "0");
	    fclose(fp);
	  }


	  if (btnPushed.size() <= 0)
	    break;

	  if (isContain(btnPushed, 200))
	    {
	      clearConsole();
	      printf("ERROR: SET PIN CODE"); fflush(stdout);
	      sleep(1);
	      break;
	    }

	  clearConsole();
	  printf("  "); fflush(stdout);

	  g_state = S_SET_PIN; // 本来はnextStateとかにすべきだが

	  /* FALL THROUGH */
	case S_SET_PIN: // PIN設定中
	  for (vector<int>::const_iterator it=btnPushed.begin();
	       it!=btnPushed.end(); ++it)
	    {
	      // 数字ボタンが押された
	      if (*it >= 0 && *it <= 9)
		{
		  pinCode.push_back(*it);
		  printf("* "); fflush(stdout);
		}
	    }

	  // ARM
	  if (isContain(btnPushed, 200))
	    {
	      clearConsole();
	      printf("ARMED"); fflush(stdout);
	      sleep(1);
	      g_state = S_ARMED;
	      break;
	    }

	  // Cancel
	  if (isContain(btnPushed, 100))
	    {
	      pinCode.clear();
	      clearConsole();
	      printf("CANCELED"); fflush(stdout);
	      sleep(1);
	      g_state = S_INITIAL;
	      break;
	    }

	  
	  break;

	case S_ARMED: // ON
	  trailingDot = time(NULL) - lastTick;
	  if (trailingDot > maxTrailingDot)
	    {
	      trailingDot = 0;
	      lastTick = time(NULL);
	    }
	  clearConsole();
	  printf("ACTIVE");
	  for (int i=0; i<trailingDot; ++i) putchar('.'); fflush(stdout);

	  {
	    FILE *fp = fopen("/sys/class/leds/red_led/brightness", "w");
	    fprintf(fp, "255");
	    fclose(fp);
	  }

	  if (btnPushed.size() <= 0)
	    break;

	  if (isContain(btnPushed, 200))
	    {
	      clearConsole();
	      printf("ERROR: ENTER PIN CODE"); fflush(stdout);
	      sleep(1);
	      break;
	    }

	  if (isContain(btnPushed, 100))
	    break;

	  clearConsole();
	  printf("  "); fflush(stdout);

	  verifyPos = -1;

	  g_state = S_INPUT_PIN; // 本来はnextStateとかにすべきだが

	  /* FALL THROUGH */
	case S_INPUT_PIN: // 解除PIN入力中
	  for (vector<int>::const_iterator it=btnPushed.begin();
	       it!=btnPushed.end(); ++it)
	    {
	      // 数字ボタンが押された
	      if (*it >= 0 && *it <= 9)
		{
		  ++verifyPos;
		  
		  printf("* "); fflush(stdout);

		  if (verifyPos < pinCode.size()
		      && pinCode[verifyPos] != *it)
		    {
		      verifyPos = pinCode.size() + 1;
		    }
		}

	      if (isContain(btnPushed, 200))
		{
		  if (verifyPos == pinCode.size()-1)
		    {
		      pinCode.clear();
		      
		      clearConsole();
		      printf("DISARMED"); fflush(stdout);
		      sleep(1);
		      g_state = S_INITIAL;

		      break;
		    } else
		    {
		      clearConsole();
		      printf("ERROR: INVALID PIN CODE"); fflush(stdout);
		      sleep(1);
		      g_state = S_ARMED;
		      
		      break;
		    }
		}

	      if (isContain(btnPushed, 100))
		{
		  clearConsole();
		  g_state = S_ARMED;

		  break;
		}
	    }

	  break;

	default:
	  break;
	}

      // save button state
      prev = cur;
      usleep(150 * 1000);
    }

  return 0;
}

