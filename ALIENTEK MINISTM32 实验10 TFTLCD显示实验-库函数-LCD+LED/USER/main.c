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


//UCOSIII中以下优先级用户程序不能使用，ALIENTEK
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		512
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//任务优先级
#define LED0_TASK_PRIO		4
//任务堆栈大小	
#define LED0_STK_SIZE 		128
//任务控制块
OS_TCB Led0TaskTCB;
//任务堆栈	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);


//任务优先级
#define MAIN_TASK_PRIO		8
//任务堆栈大小	
#define MAIN_STK_SIZE 		128
//任务控制块
OS_TCB Main_TaskTCB;
//任务堆栈	
CPU_STK MAIN_TASK_STK[MAIN_STK_SIZE];
void main_task(void *p_arg);


//任务优先级
#define KEY_TASK_PRIO		6
//任务堆栈大小	
#define KEY_STK_SIZE 		128
//任务控制块
OS_TCB KEYTaskTCB;
//任务堆栈	
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void Key_task(void *p_arg);


//任务优先级
#define LCD_TASK_PRIO		7
//任务堆栈大小	
#define LCD_STK_SIZE 		128
//任务控制块
OS_TCB LCDTaskTCB;
//任务堆栈	
CPU_STK LCD_TASK_STK[LCD_STK_SIZE];
//任务函数
void LCD_task(void *p_arg);




////////////////////////////////////////////////////////
u8 tmr1sta=0; 	//标记定时器的工作状态
OS_TMR 	tmr1;		//定时器1
void tmr1_callback(void *p_tmr, void *p_arg); 	//定时器1回调函数


#define KEYMSG_Q_NUM 		1 //按键消息队列的数量
#define DATAMSG_Q_NUM 		4 //发送数据的消息队列的数量
OS_Q KEY_Msg; //定义一个消息队列，用于按键消息传递，模拟消息邮箱
OS_Q DATA_Msg; //定义一个消息队列，用于发送数据




//Mini STM32开发板范例代码10
//TFTLCD显示 实验
//正点原子@ALIENTEK
//技术论坛:www.openedv.com	
 int main(void)
 {
	OS_ERR err;
	CPU_SR_ALLOC();
	 

	SystemInit();
	delay_init();	     //延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //中断分组配置
	uart_init(9600);
 	LED_Init();
	KEY_Init();
	LCD_Init();
	POINT_COLOR=RED;
	 
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区 
	 
	 //创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
	while(1);
	 
 }	 
	 
//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//进入临界区
	//创建LED0任务
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
	
		//创建Key任务
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
			//创建LCD任务
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
	//创建主任务
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
				 
	//创建消息队列 KEY_Msg
	OSQCreate ( (OS_Q* )&KEY_Msg,		//消息队列 
			(CPU_CHAR* )"KEY Msg",		//消息队列名称
			(OS_MSG_QTY )KEYMSG_Q_NUM,  //消息队列长度，这里设置为 1
			(OS_ERR* )&err); 			//错误码
	//创建消息队列 DATA_Msg
	OSQCreate ( (OS_Q* )&DATA_Msg, 
			(CPU_CHAR* )"DATA Msg",
			(OS_MSG_QTY )DATAMSG_Q_NUM,
			(OS_ERR* )&err);	
				 
	//创建定时器1
	OSTmrCreate((OS_TMR		*)&tmr1,		//定时器1
                (CPU_CHAR	*)"tmr1",		//定时器名字
                (OS_TICK	 )20,			//20*10=200ms
                (OS_TICK	 )100,          //100*10=1000ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//定时器1回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码			 
	OSTmrStart(&tmr1,&err);	//开启定时器1			 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//进入临界区
				 
}	

//LED0任务函数
void led0_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	while(1)
	{
		LED0=0;
		OSTimeDlyHMSM(0,0,0,200,OS_OPT_TIME_HMSM_STRICT,&err); //延时200ms
		LED0=1;
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //延时500ms

	}
}	
	 
	
	 
//Key任务函数
void Key_task(void *p_arg)
{

	u8 *key;
	OS_MSG_SIZE size;
	OS_ERR err;
	while(1)
	{		
		//请求消息 KEY_Msg
		key=OSQPend((OS_Q* )&KEY_Msg,
					(OS_TICK )0,
					(OS_OPT )OS_OPT_PEND_BLOCKING,
					(OS_MSG_SIZE* )&size,
					(CPU_TS* )0,
					(OS_ERR* )&err);
		switch(*key)
		{
			case Button_WAKEUP : //KEY_UP 控制 LED1
				LED1 = ~LED1;
				LCD_ShowString(50,170,"WAKEUP!");
			break;
			case Button_KEY0: //KEY0 刷新 LCD 背景
				LED0= ~LED0;
				LCD_ShowString(50,170,"KEY0");
			break;
			case Button_KEY1: //KEY1 控制定时器 1
				tmr1sta = !tmr1sta;
				if(tmr1sta)
				{
					OSTmrStart(&tmr1,&err);
					LCD_ShowString(50,170,"TMR1 START!");
				}
				else
				{
					OSTmrStop(&tmr1,OS_OPT_TMR_NONE,0,&err); //停止定时器 1
					LCD_ShowString(50,170,"TMR1 STOP! ");
				}
			break;
		}	
	}

}	

	//LCD任务函数
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
		OS_CRITICAL_ENTER();	//进入临界区 
		LCD_ShowString(30,50,"Mini STM32 ^_^");	
		LCD_ShowString(30,70,"2.4'/2.8' TFTLCD TEST");	
		LCD_ShowString(30,90,"ATOM@ALIENTEK");
		LCD_ShowString(30,110,"2010/12/30");	
			OS_CRITICAL_EXIT();		//退出临界区    					 
	    x++;
		if(x==12)x=0;
		LED0=!LED0;					 
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s
	}	

 }

//定时器1的回调函数
void tmr1_callback(void *p_tmr, void *p_arg)
{
	static u8 tmr1_num=0;
	LCD_ShowString(30,130,"Timer int");	
	LCD_ShowNum(182,131,tmr1_num,3,16);  //显示定时器1执行次数	
	tmr1_num++;		//定时器1执行次数加1
}

void main_task(void *p_arg)
{
	u8 key;
	OS_ERR err;
	while(1)
	{
		key = KEY_Scan();  //扫描按键
		if(key)
		{
			//发送消息
			OSQPost((OS_Q*		)&KEY_Msg,		
					(void*		)&key,
					(OS_MSG_SIZE)1,
					(OS_OPT		)OS_OPT_POST_FIFO,
					(OS_ERR*	)&err);
		}
		
		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);   //延时10ms
	}
}


