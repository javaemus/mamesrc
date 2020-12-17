#ifndef AY8910INTF_H
#define AY8910INTF_H


#define MAX_8910 5


struct AY8910interface
{
	int num;	/* total number of 8910 in the machine */
	int updates_per_frame;	/* you have to call AY8910_update() this number of */
							/* times each video frame. This is usually done from */
							/* inside the interrupt handler. If you set this to */
							/* 1, you don't HAVE /you may if you want to) to call */
							/* AY8910_update(), it will be called automatically by */
							/* AY8910_sh_update() */
	int clock;
	int volume[MAX_8910];
	int (*portAread[MAX_8910])(int offset);
	int (*portBread[MAX_8910])(int offset);
	void (*portAwrite[MAX_8910])(int offset,int data);
	void (*portBwrite[MAX_8910])(int offset,int data);
};



int AY8910_read_port_0_r(int offset);
int AY8910_read_port_1_r(int offset);
int AY8910_read_port_2_r(int offset);
int AY8910_read_port_3_r(int offset);
int AY8910_read_port_4_r(int offset);

void AY8910_control_port_0_w(int offset,int data);
void AY8910_control_port_1_w(int offset,int data);
void AY8910_control_port_2_w(int offset,int data);
void AY8910_control_port_3_w(int offset,int data);
void AY8910_control_port_4_w(int offset,int data);

void AY8910_write_port_0_w(int offset,int data);
void AY8910_write_port_1_w(int offset,int data);
void AY8910_write_port_2_w(int offset,int data);
void AY8910_write_port_3_w(int offset,int data);
void AY8910_write_port_4_w(int offset,int data);

void AY8910_update(void);

int AY8910_sh_start(struct AY8910interface *interface);
void AY8910_sh_stop(void);
void AY8910_sh_update(void);

#endif
