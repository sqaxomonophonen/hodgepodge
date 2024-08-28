int console_init(void);
void console_shutdown(void);
void console_toggle(void);
void console_mainloop(void);
void console_printf(const char *, ...);
void console_process_key(SDL_keysym *);
void console_parse_string(const char *);
