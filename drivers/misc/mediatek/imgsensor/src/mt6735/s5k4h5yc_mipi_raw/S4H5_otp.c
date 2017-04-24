

struct Darling_SK4H5_otp_struct {
int flag; 
int MID;
int LID;
int RGr_ratio;
int BGr_ratio;
int GbGr_ratio;
int VCM_start;
int VCM_end;
} Darling_SK4H5_OTP;

#define RGr_ratio_Typical    256
#define BGr_ratio_Typical    256
#define GbGr_ratio_Typical  512

#define SK4H5_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, imgsensor.i2c_write_id)

//extern  void SK4H5_write_cmos_sensor(u16 addr, u32 para);
//extern  unsigned char SK4H5_read_cmos_sensor(u32 addr);
static kal_uint16 SK4H5_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,imgsensor.i2c_write_id);
    return get_byte;
}

void Darling_SK4H5_read_OTP(struct Darling_SK4H5_otp_struct *SK4H5_OTP)
{
     SK4H5_write_cmos_sensor(0x3A02,0x00);
     SK4H5_write_cmos_sensor(0x3A00,0x01);
     mdelay(5);
     unsigned char val=0;
     val=SK4H5_read_cmos_sensor(0x3A04);//flag of info and awb
	 if(val==0x40)
	 	{
	 	SK4H5_OTP->flag = 0x01;
		SK4H5_OTP->MID = SK4H5_read_cmos_sensor(0x3A05);
		SK4H5_OTP->LID = SK4H5_read_cmos_sensor(0x3A06);
		val = SK4H5_read_cmos_sensor(0x3A0A);
		SK4H5_OTP->RGr_ratio = (SK4H5_read_cmos_sensor(0x3A07)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->BGr_ratio = (SK4H5_read_cmos_sensor(0x3A08)<<2)+((val&0x30)>>4);
		SK4H5_OTP->GbGr_ratio = (SK4H5_read_cmos_sensor(0x3A09)<<2)+((val&0x0c)>>2);
	 	}
	 else if(val==0xd0)
	 	{
	 	SK4H5_OTP->flag=0x01;
		SK4H5_OTP->MID = SK4H5_read_cmos_sensor(0x3A0B);
		SK4H5_OTP->LID = SK4H5_read_cmos_sensor(0x3A0C);
		val = SK4H5_read_cmos_sensor(0x3A10);
		SK4H5_OTP->RGr_ratio = (SK4H5_read_cmos_sensor(0x3A0D)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->BGr_ratio = (SK4H5_read_cmos_sensor(0x3A0E)<<2)+((val&0x30)>>4);
		SK4H5_OTP->GbGr_ratio = (SK4H5_read_cmos_sensor(0x3A0F)<<2)+((val&0x0c)>>2);
	 	}
	 else
	 	{
	 	SK4H5_OTP->flag=0x00;
		SK4H5_OTP->MID =0x00;
		SK4H5_OTP->LID = 0x00;
		SK4H5_OTP->RGr_ratio = 0x00;
		SK4H5_OTP->BGr_ratio = 0x00;
		SK4H5_OTP->GbGr_ratio = 0x00;
	 	}

	 val=SK4H5_read_cmos_sensor(0x3A11);//falg of VCM
       if(val==0x40)
	 	{
	 	SK4H5_OTP->flag += 0x04;
		val = SK4H5_read_cmos_sensor(0x3A14);
		SK4H5_OTP->VCM_start= (SK4H5_read_cmos_sensor(0x3A12)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->VCM_end = (SK4H5_read_cmos_sensor(0x3A13)<<2)+((val&0x30)>>4);

	 	}
	 else if(val==0xd0)
	 	{
	 	SK4H5_OTP->flag+=0x04;
		val = SK4H5_read_cmos_sensor(0x3A17);
		SK4H5_OTP->VCM_start= (SK4H5_read_cmos_sensor(0x3A15)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->VCM_end = (SK4H5_read_cmos_sensor(0x3A16)<<2)+((val&0x30)>>4);
	 	}
	 else
	 	{
	 	SK4H5_OTP->flag+=0x00;
		SK4H5_OTP->VCM_start= 0x00;
		SK4H5_OTP->VCM_end = 0x00;
	 	}
	 
	 SK4H5_write_cmos_sensor(0x3A00,0x00);
}

void Darling_SK4H5_apply_OTP(struct Darling_SK4H5_otp_struct *SK4H5_OTP)
{
   if(((SK4H5_OTP->flag)&0x03)!=0x01) 	return;
   
   	int R_gain,B_gain,Gb_gain,Gr_gain,Base_gain;
	R_gain = (RGr_ratio_Typical*1000)/SK4H5_OTP->RGr_ratio;
	B_gain = (BGr_ratio_Typical*1000)/SK4H5_OTP->BGr_ratio;
	Gb_gain = (GbGr_ratio_Typical*1000)/SK4H5_OTP->GbGr_ratio;
	Gr_gain = 1000;
       Base_gain = R_gain;
	if(Base_gain>B_gain) Base_gain=B_gain;
	if(Base_gain>Gb_gain) Base_gain=Gb_gain;
	if(Base_gain>Gr_gain) Base_gain=Gr_gain;
	R_gain = 0x100 * R_gain /Base_gain;
	B_gain = 0x100 * B_gain /Base_gain;
	Gb_gain = 0x100 * Gb_gain /Base_gain;
	Gr_gain = 0x100 * Gr_gain /Base_gain;
	
	printk("Gr_gain=0x%x, R_gain=0x%x, B_gain=0x%x, Gb_gain=0x%x\n",Gr_gain, R_gain, B_gain, Gb_gain);
	if(Gr_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x020e,Gr_gain>>8);
                   SK4H5_write_cmos_sensor(0x020f,Gr_gain&0xff);
		}
	if(R_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0210,R_gain>>8);
                   SK4H5_write_cmos_sensor(0x0211,R_gain&0xff);
		}
	if(B_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0212,B_gain>>8);
                   SK4H5_write_cmos_sensor(0x0213,B_gain&0xff);
		}
	if(Gb_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0214,Gb_gain>>8);
                   SK4H5_write_cmos_sensor(0x0215,Gb_gain&0xff);
		}
}


void read_otp(void)
{
	Darling_SK4H5_read_OTP(&Darling_SK4H5_OTP);
}

void apply_otp(void)
{
	Darling_SK4H5_apply_OTP(&Darling_SK4H5_OTP);
}
