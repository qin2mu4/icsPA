#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

extern void switch_game();

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  // return 0;
  int key = _read_key();
  char keydown_char = (key&0x8000 ? 'd' : 'u');//通码=断码+0x8000
  int keyid = key & ~0x8000;
  if(keyid != _KEY_NONE) {//优先处理按键事件
    snprintf(buf, len, "k%c %s\n", keydown_char, keyname[keyid]);//写入按键事件
    if ((key & 0x8000) && (keyid == _KEY_F12)) {
      switch_game();//切换游戏
    }
    return strlen(buf);
  }
  else {
    unsigned long time_ms=_uptime();
    return snprintf(buf, len, "t %d\n", time_ms) - 1;//写入时间
  }
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  offset /= sizeof(uint32_t);//计算屏幕上的坐标
  //把 buf 中的 len 字节写到屏幕上 offset 处
  _draw_rect(buf, offset % _screen.width, offset / _screen.width, len / sizeof(uint32_t), 1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH: %d\nHEIGHT: %d", _screen.width, _screen.height);
}
