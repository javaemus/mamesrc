#ifndef OSDEPEND_H
#define OSDEPEND_H


struct osd_bitmap
{
	int width,height;	/* width and hegiht of the bitmap */
	void *_private;	/* don't touch! - reserved for osdepend use */
	unsigned char *line[0];	/* pointers to the start of each line */
};


#define OSD_KEY_ESC         1        /* keyboard scan codes */
#define OSD_KEY_1           2        /* (courtesy of allegro.h) */
#define OSD_KEY_2           3
#define OSD_KEY_3           4
#define OSD_KEY_4           5
#define OSD_KEY_5           6
#define OSD_KEY_6           7
#define OSD_KEY_7           8
#define OSD_KEY_8           9
#define OSD_KEY_9           10
#define OSD_KEY_0           11
#define OSD_KEY_MINUS       12
#define OSD_KEY_EQUALS      13
#define OSD_KEY_BACKSPACE   14
#define OSD_KEY_TAB         15
#define OSD_KEY_Q           16
#define OSD_KEY_W           17
#define OSD_KEY_E           18
#define OSD_KEY_R           19
#define OSD_KEY_T           20
#define OSD_KEY_Y           21
#define OSD_KEY_U           22
#define OSD_KEY_I           23
#define OSD_KEY_O           24
#define OSD_KEY_P           25
#define OSD_KEY_OPENBRACE   26
#define OSD_KEY_CLOSEBRACE  27
#define OSD_KEY_ENTER       28
#define OSD_KEY_LCONTROL    29
#define OSD_KEY_A           30
#define OSD_KEY_S           31
#define OSD_KEY_D           32
#define OSD_KEY_F           33
#define OSD_KEY_G           34
#define OSD_KEY_H           35
#define OSD_KEY_J           36
#define OSD_KEY_K           37
#define OSD_KEY_L           38
#define OSD_KEY_COLON       39
#define OSD_KEY_QUOTE       40
#define OSD_KEY_TILDE       41
#define OSD_KEY_LSHIFT      42
/* 43 */
#define OSD_KEY_Z           44
#define OSD_KEY_X           45
#define OSD_KEY_C           46
#define OSD_KEY_V           47
#define OSD_KEY_B           48
#define OSD_KEY_N           49
#define OSD_KEY_M           50
#define OSD_KEY_COMMA       51
#define OSD_KEY_STOP        52
#define OSD_KEY_SLASH       53
#define OSD_KEY_RSHIFT      54
#define OSD_KEY_ASTERISK    55
#define OSD_KEY_ALT         56
#define OSD_KEY_SPACE       57
#define OSD_KEY_CAPSLOCK    58
#define OSD_KEY_F1          59
#define OSD_KEY_F2          60
#define OSD_KEY_F3          61
#define OSD_KEY_F4          62
#define OSD_KEY_F5          63
#define OSD_KEY_F6          64
#define OSD_KEY_F7          65
#define OSD_KEY_F8          66
#define OSD_KEY_F9          67
#define OSD_KEY_F10         68
#define OSD_KEY_NUMLOCK     69
#define OSD_KEY_SCRLOCK     70
#define OSD_KEY_HOME        71
#define OSD_KEY_UP          72
#define OSD_KEY_PGUP        73
#define OSD_KEY_MINUS_PAD   74
#define OSD_KEY_LEFT        75
#define OSD_KEY_5_PAD       76
#define OSD_KEY_RIGHT       77
#define OSD_KEY_PLUS_PAD    78
#define OSD_KEY_END         79
#define OSD_KEY_DOWN        80
#define OSD_KEY_PGDN        81
#define OSD_KEY_INSERT      82
#define OSD_KEY_DEL         83
#define OSD_KEY_RCONTROL    84	/* different from Allegro */
#define OSD_KEY_ALTGR       85	/* different from Allegro */
/* 86 */
#define OSD_KEY_F11         87
#define OSD_KEY_F12         88
#define OSD_MAX_KEY         88


#define OSD_JOY_LEFT	1
#define OSD_JOY_RIGHT	2
#define OSD_JOY_UP	3
#define OSD_JOY_DOWN	4
#define OSD_JOY_FIRE1	5
#define OSD_JOY_FIRE2	6
#define OSD_JOY_FIRE3	7
#define OSD_JOY_FIRE4	8
#define OSD_JOY_FIRE5	9
#define OSD_JOY_FIRE6	10
#define OSD_JOY_FIRE7	11
#define OSD_JOY_FIRE8	12
#define OSD_JOY_FIRE9	13
#define OSD_JOY_FIRE10	14
#define OSD_JOY_FIRE	15	/* any of the first joystick fire buttons */
#define OSD_JOY2_LEFT	16
#define OSD_JOY2_RIGHT	17
#define OSD_JOY2_UP	18
#define OSD_JOY2_DOWN	19
#define OSD_JOY2_FIRE1	20
#define OSD_JOY2_FIRE2	21
#define OSD_JOY2_FIRE3	22
#define OSD_JOY2_FIRE4	23
#define OSD_JOY2_FIRE5	24
#define OSD_JOY2_FIRE6	25
#define OSD_JOY2_FIRE7	26
#define OSD_JOY2_FIRE8	27
#define OSD_JOY2_FIRE9	28
#define OSD_JOY2_FIRE10	29
#define OSD_JOY2_FIRE	30	/* any of the second joystick fire buttons */
#define OSD_MAX_JOY     30

#define X_AXIS          1
#define Y_AXIS          2
#define NO_TRAK         1000000


extern int play_sound;
extern int video_sync;


int osd_init(int argc,char **argv);
void osd_exit(void);
struct osd_bitmap *osd_create_bitmap(int width,int height);
void osd_clearbitmap(struct osd_bitmap *bitmap);
void osd_free_bitmap(struct osd_bitmap *bitmap);
/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int totalcolors,
		const unsigned char *palette,unsigned char *pens,int attributes);
void osd_close_display(void);
void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue);
void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue);
void osd_mark_dirty(int x1, int y1, int x2, int y2, int ui);	/* ASG 971011 */
void osd_update_display(void);
void osd_update_audio(void);
void osd_play_sample(int channel,unsigned char *data,int len,int freq,int volume,int loop);
void osd_play_streamed_sample(int channel,unsigned char *data,int len,int freq,int volume);
void osd_adjust_sample(int channel,int freq,int volume);
void osd_stop_sample(int channel);
void osd_restart_sample(int channel);
int osd_get_sample_status(int channel);
void osd_ym2203_write(int n, int r, int v);
void osd_ym2203_update(void);
void osd_set_mastervolume(int volume);
int osd_key_pressed(int keycode);
int osd_read_key(void);
int osd_read_keyrepeat(void);
const char *osd_joy_name(int joycode);
const char *osd_key_name(int keycode);
void osd_poll_joystick(void);
int osd_joy_pressed(int joycode);

int osd_trak_read(int axis);

int osd_update_vectors(int *x_res, int *y_res, int step);
void osd_draw_to(int x, int y, int col);


/* file handling routines */
#define OSD_FILETYPE_ROM 1
#define OSD_FILETYPE_SAMPLE 2
#define OSD_FILETYPE_HIGHSCORE 3
#define OSD_FILETYPE_CONFIG 4

/* gamename holds the driver name, filename is only used for ROMs and samples. */
/* if 'write' is not 0, the file is opened for write. Otherwise it is opened */
/* for read. */
void *osd_fopen (const char *gamename,const char *filename,int filetype,int write);
int osd_fread (void *file,void *buffer,int length);
int osd_fwrite (void *file,const void *buffer,int length);
int osd_fseek (void *file,int offset,int whence);
void osd_fclose (void *file);

/* control keyboard leds or other indicators */
void osd_led_w(int led,int on);

#endif
