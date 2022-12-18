#include "led.h"
#include "delay.h"
#include "sys.h"
#include "key.h"
#include "usart.h"
#include "exti.h"
#include "wdg.h"
#include "timer.h"
#include "pwm.h"
#include "lcd.h"

#include "includes.h"


//UCOSIII���������ȼ��û�������ʹ�ã�ALIENTEK
//����Щ���ȼ��������UCOSIII��5��ϵͳ�ڲ�����
//���ȼ�0���жϷ������������� OS_IntQTask()
//���ȼ�1��ʱ�ӽ������� OS_TickTask()
//���ȼ�2����ʱ���� OS_TmrTask()
//���ȼ�OS_CFG_PRIO_MAX-2��ͳ������ OS_StatTask()
//���ȼ�OS_CFG_PRIO_MAX-1���������� OS_IdleTask()

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		512
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//�������ȼ�
#define LED0_TASK_PRIO		4
//�����ջ��С	
#define LED0_STK_SIZE 		128
//������ƿ�
OS_TCB Led0TaskTCB;
//�����ջ	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);


//�������ȼ�
#define MAIN_TASK_PRIO		8
//�����ջ��С	
#define MAIN_STK_SIZE 		128
//������ƿ�
OS_TCB Main_TaskTCB;
//�����ջ	
CPU_STK MAIN_TASK_STK[MAIN_STK_SIZE];
void main_task(void *p_arg);


//�������ȼ�
#define KEY_TASK_PRIO		6
//�����ջ��С	
#define KEY_STK_SIZE 		128
//������ƿ�
OS_TCB KEYTaskTCB;
//�����ջ	
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
//������
void Key_task(void *p_arg);


//�������ȼ�
#define LCD_TASK_PRIO		7
//�����ջ��С	
#define LCD_STK_SIZE 		128
//������ƿ�
OS_TCB LCDTaskTCB;
//�����ջ	
CPU_STK LCD_TASK_STK[LCD_STK_SIZE];
//������
void LCD_task(void *p_arg);




////////////////////////////////////////////////////////
u8 tmr1sta=0; 	//��Ƕ�ʱ���Ĺ���״̬
OS_TMR 	tmr1;		//��ʱ��1
void tmr1_callback(void *p_tmr, void *p_arg); 	//��ʱ��1�ص�����


#define KEYMSG_Q_NUM 		1 //������Ϣ���е�����
#define DATAMSG_Q_NUM 		4 //�������ݵ���Ϣ���е�����
OS_Q KEY_Msg; //����һ����Ϣ���У����ڰ�����Ϣ���ݣ�ģ����Ϣ����
OS_Q DATA_Msg; //����һ����Ϣ���У����ڷ�������




//Mini STM32�����巶������10
//TFTLCD��ʾ ʵ��
//����ԭ��@ALIENTEK
//������̳:www.openedv.com	
 int main(void)
 {
	OS_ERR err;
	CPU_SR_ALLOC();
	 

	SystemInit();
	delay_init();	     //��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //�жϷ�������
	uart_init(9600);
 	LED_Init();
	KEY_Init();
	LCD_Init();
	POINT_COLOR=RED;
	 
	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ��� 
	 
	 //������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
	while(1);
	 
 }	 
	 
//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//�����ٽ���
	//����LED0����
	OSTaskCreate((OS_TCB 	* )&Led0TaskTCB,		
				 (CPU_CHAR	* )"led0 task", 		
                 (OS_TASK_PTR )led0_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED0_TASK_PRIO,     
                 (CPU_STK   * )&LED0_TASK_STK[0],	
                 (CPU_STK_SIZE)LED0_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED0_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);	
	
		//����Key����
	OSTaskCreate((OS_TCB 	* )&KEYTaskTCB,		
				 (CPU_CHAR	* )"KEY task", 		
                 (OS_TASK_PTR )Key_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )KEY_TASK_PRIO,     	
                 (CPU_STK   * )&KEY_TASK_STK[0],	
                 (CPU_STK_SIZE)KEY_STK_SIZE/10,	
                 (CPU_STK_SIZE)KEY_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);			 
			//����LCD����
	OSTaskCreate((OS_TCB 	* )&LCDTaskTCB,		
				 (CPU_CHAR	* )"LCD task", 		
                 (OS_TASK_PTR )LCD_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LCD_TASK_PRIO,     	
                 (CPU_STK   * )&LCD_TASK_STK[0],	
                 (CPU_STK_SIZE)LCD_STK_SIZE/10,	
                 (CPU_STK_SIZE)LCD_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
	//����������
	OSTaskCreate((OS_TCB 	* )&Main_TaskTCB,		
				 (CPU_CHAR	* )"Main task", 		
                 (OS_TASK_PTR )main_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MAIN_TASK_PRIO,     
                 (CPU_STK   * )&MAIN_TASK_STK[0],	
                 (CPU_STK_SIZE)MAIN_STK_SIZE/10,	
                 (CPU_STK_SIZE)MAIN_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);
				 
	//������Ϣ���� KEY_Msg
	OSQCreate ( (OS_Q* )&KEY_Msg,		//��Ϣ���� 
			(CPU_CHAR* )"KEY Msg",		//��Ϣ��������
			(OS_MSG_QTY )KEYMSG_Q_NUM,  //��Ϣ���г��ȣ���������Ϊ 1
			(OS_ERR* )&err); 			//������
	//������Ϣ���� DATA_Msg
	OSQCreate ( (OS_Q* )&DATA_Msg, 
			(CPU_CHAR* )"DATA Msg",
			(OS_MSG_QTY )DATAMSG_Q_NUM,
			(OS_ERR* )&err);	
				 
	//������ʱ��1
	OSTmrCreate((OS_TMR		*)&tmr1,		//��ʱ��1
                (CPU_CHAR	*)"tmr1",		//��ʱ������
                (OS_TICK	 )20,			//20*10=200ms
                (OS_TICK	 )100,          //100*10=1000ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//��ʱ��1�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����			 
	OSTmrStart(&tmr1,&err);	//������ʱ��1			 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	OS_CRITICAL_EXIT();	//�����ٽ���
				 
}	

//LED0������
void led0_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	while(1)
	{
		LED0=0;
		OSTimeDlyHMSM(0,0,0,200,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ200ms
		LED0=1;
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ500ms

	}
}	
	 
	
	 
//Key������
void Key_task(void *p_arg)
{

	u8 *key;
	OS_MSG_SIZE size;
	OS_ERR err;
	while(1)
	{		
		//������Ϣ KEY_Msg
		key=OSQPend((OS_Q* )&KEY_Msg,
					(OS_TICK )0,
					(OS_OPT )OS_OPT_PEND_BLOCKING,
					(OS_MSG_SIZE* )&size,
					(CPU_TS* )0,
					(OS_ERR* )&err);
		switch(*key)
		{
			case Button_WAKEUP : //KEY_UP ���� LED1
				LED1 = ~LED1;
				LCD_ShowString(50,170,"WAKEUP!");
			break;
			case Button_KEY0: //KEY0 ˢ�� LCD ����
				LED0= ~LED0;
				LCD_ShowString(50,170,"KEY0");
			break;
			case Button_KEY1: //KEY1 ���ƶ�ʱ�� 1
				tmr1sta = !tmr1sta;
				if(tmr1sta)
				{
					OSTmrStart(&tmr1,&err);
					LCD_ShowString(50,170,"TMR1 START!");
				}
				else
				{
					OSTmrStop(&tmr1,OS_OPT_TMR_NONE,0,&err); //ֹͣ��ʱ�� 1
					LCD_ShowString(50,170,"TMR1 STOP! ");
				}
			break;
		}	
	}

}	

	//LCD������
void LCD_task(void *p_arg)
{
CPU_SR_ALLOC();
	OS_ERR err;
 	u8 x=0;
	while(1) 
	{		 
		switch(x)
		{
			case 0:LCD_Clear(WHITE);break;
			case 1:LCD_Clear(BLACK);break;
			case 2:LCD_Clear(BLUE);break;
			case 3:LCD_Clear(RED);break;
			case 4:LCD_Clear(MAGENTA);break;
			case 5:LCD_Clear(GREEN);break;
			case 6:LCD_Clear(CYAN);break;

			case 7:LCD_Clear(YELLOW);break;
			case 8:LCD_Clear(BRRED);break;
			case 9:LCD_Clear(GRAY);break;
			case 10:LCD_Clear(LGRAY);break;
			case 11:LCD_Clear(BROWN);break;
		}
		POINT_COLOR=RED;	 
		OS_CRITICAL_ENTER();	//�����ٽ��� 
		LCD_ShowString(30,50,"Mini STM32 ^_^");	
		LCD_ShowString(30,70,"2.4'/2.8' TFTLCD TEST");	
		LCD_ShowString(30,90,"ATOM@ALIENTEK");
		LCD_ShowString(30,110,"2010/12/30");	
			OS_CRITICAL_EXIT();		//�˳��ٽ���    					 
	    x++;
		if(x==12)x=0;
		LED0=!LED0;					 
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s
	}	

 }

//��ʱ��1�Ļص�����
void tmr1_callback(void *p_tmr, void *p_arg)
{
	static u8 tmr1_num=0;
	LCD_ShowString(30,130,"Timer int");	
	LCD_ShowNum(182,131,tmr1_num,3,16);  //��ʾ��ʱ��1ִ�д���	
	tmr1_num++;		//��ʱ��1ִ�д�����1
}

void main_task(void *p_arg)
{
	u8 key;
	OS_ERR err;
	while(1)
	{
		key = KEY_Scan();  //ɨ�谴��
		if(key)
		{
			//������Ϣ
			OSQPost((OS_Q*		)&KEY_Msg,		
					(void*		)&key,
					(OS_MSG_SIZE)1,
					(OS_OPT		)OS_OPT_POST_FIFO,
					(OS_ERR*	)&err);
		}
		
		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);   //��ʱ10ms
	}
}


