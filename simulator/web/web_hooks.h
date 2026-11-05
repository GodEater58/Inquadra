/* web_hooks.h - funzioni della demo nel browser chiamate dal simulatore (vedi web.c) */
struct tm;
int web_str_width(int font_id, const char *s);
void web_text(int fontspec, int x, int y, const char *s);
void web_rect(int c, int x, int y, int w, int h);
void web_fill_circle(int x, int y, int r, int c);
void web_circle(int x, int y, int r, int c);
void web_line(int x1, int y1, int x2, int y2, int c);
void web_sleep(int ms);
int web_ms(void);
void web_task(const char *name, void (*f)(void *), void *arg);
void web_now(struct tm *t);
const char *web_fmt_shutter(int raw);
