/**
 ******************************************************************************
  * File Name          : task_scheduler.h
  * Description        : This file provides accurate super loop task timer 
												 scheduler.
  ******************************************************************************
  *
  * Copyright (c) 2017 BATKON A.S. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY BATKON A.S.
	* Author: Turker SAHIN
  ******************************************************************************
  */
#ifndef TASK_SCHEDULER_H_
#define TASK_SCHEDULER_H_

#ifdef __cplusplus
 extern "C" {
#endif

/*******************************************************************************/
/*						     								Includes							 											 */
/*******************************************************************************/
#include "stm32f4xx_hal.h"
#include <stdint.h>

/*******************************************************************************/
/*						     							Defininitions							 										 */
/*******************************************************************************/

#define NUM_OF_TASK_QUEUE		5
#define GET_TICK()					HAL_GetTick()
#define TSCH_DELAY(x)				HAL_Delay(x)

#define log_debug						printf


#if !defined(BOOLEAN)
	typedef uint8_t BOOLEAN;
	#define FALSE		(0)
	#define TRUE		(~FALSE)
#endif

typedef enum {
	TP_HIGHEST=1,
	TP_HIGHER,
	TP_NORMAL,
	TP_LOWER,
	TP_LOWEST,
}TaskPriority_t;

typedef enum {
	TS_IDLE=1,
	TS_READY,
	TS_RULE,
	TS_EXEC_COUNT,
	TS_RUNNING,
	TS_FINISHED,
	TS_SUSPENDED,
	TS_KILLED,
}TaskStatus_t;

typedef struct Task_t Task_t;
typedef void (*TaskCallbackHandler)(volatile Task_t *task);

typedef int 		(*TaskJobFunc)(volatile Task_t *task);
typedef BOOLEAN (*TaskRuleFunc)(volatile Task_t *task);


struct Task_t {
	int 								Index;
	uint32_t 						Period;
	uint32_t 						BeginTime;
	uint32_t 						BeginExecTime;
	uint32_t 						ExecutionTime;
	uint32_t 						ExecutionExecTime;
	int 								ExecutionCount;
	uint32_t 						EndTime;
	int 								NumOfExecution;
	uint32_t 						NextScheduleTime;
	TaskPriority_t			Priority;
	TaskStatus_t 				Status;
	BOOLEAN							Suspend;
	char*								JobName;
	TaskJobFunc					Job;
	TaskRuleFunc				Rule;
	int									ReturnCode;
	int									StartupDelay;
	TaskCallbackHandler CallbackHandler;
	BOOLEAN							Statistics;
	BOOLEAN							TimeCorrection;
};

typedef struct {
	int						TaskIndex;
	uint32_t 			Delay;
	uint32_t 			Tick;
	Task_t			 	Task[NUM_OF_TASK_QUEUE];
	BOOLEAN				Statistics;
}TaskScheduler_t;


/*******************************************************************************/
/*						     					 Function Prototypes						 									 */
/*******************************************************************************/

void task_scheduler_init(BOOLEAN statOn, uint32_t delay);

volatile Task_t* task_scheduler_add(TaskJobFunc job, char *jobName, TaskRuleFunc rule, TaskPriority_t priority, int numOfExec, uint32_t period, uint32_t startupDelay, TaskCallbackHandler callback, BOOLEAN statDisable);
	
void task_start(volatile Task_t *task);

void task_restart(volatile Task_t *task);

void task_start_all(void);

void task_start_immediately(volatile Task_t *task);

void task_suspend(volatile Task_t *task);

void task_kill(volatile Task_t *task);

void task_scheduler_start(void);

void task_scheduler_stop(void);

#ifdef __cplusplus
}
#endif

#endif
