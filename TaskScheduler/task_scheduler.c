/**
 ******************************************************************************
  * File Name          : task_scheduler.c
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
  * Author: Turker SAHIN
  ******************************************************************************
  */
	
/*******************************************************************************/
/*						     								Includes							 										 	 */
/*******************************************************************************/
#include "task_scheduler.h"


/*******************************************************************************/
/*						     							Local variables							 									 */
/*******************************************************************************/
static volatile TaskScheduler_t taskScheduler;
static BOOLEAN startFlag = FALSE;

/*******************************************************************************/
/*						     								 Prototypes						 										 	 */
/*******************************************************************************/
void task_execute(volatile Task_t *task);

/*******************************************************************************/
/*						     									Functions							 										 */
/*******************************************************************************/

__weak void task_stats(volatile Task_t *task) {
#define TASK_MINIMAL_LOG
#ifndef TASK_MINIMAL_LOG
	log_debug("\r\nTask-%d \"%s\" Status\r\n", task->Index, task->JobName);
	log_debug("===========================================\r\n");
	log_debug("Period........:%d\r\n", task->Period);
	log_debug("Priority......:%d\r\n", task->Priority);
	log_debug("Suspended.....:%d\r\n", task->Suspend);
	log_debug("Status........:%d\r\n", task->Status);
	log_debug("Num.Of Exec...:%d\r\n", task->NumOfExecution);
	log_debug("...............\r\n");
	log_debug("Begin Time....:%d\r\n", task->BeginTime);
	log_debug("End Time......:%d\r\n", task->EndTime);
	log_debug("Execution Time:%d\r\n", task->ExecutionTime);
	log_debug("Begin Exec....:%d\r\n", task->BeginExecTime);
	log_debug("Exec Execution:%d\r\n", task->ExecutionExecTime);
	log_debug("Return Code...:%d\r\n", task->ReturnCode);
	log_debug("Next Time.....:%d\r\n", task->NextScheduleTime);
	log_debug("Exec Count....:%d\r\n", task->ExecutionCount);
	log_debug("===========================================\r\n");
#else
	log_debug("\r\nT-%d \"%s\"\r\n", task->Index, task->JobName);
	log_debug("\r\nB:%d\r\n", task->BeginExecTime);
	log_debug("E:%d\r\n", task->ExecutionTime);
	log_debug("N:%d\r\n", task->NextScheduleTime);
#endif
};

static void task_fire_event(volatile Task_t *task)
{
	// if handler is null do not try to call handler.
	if(task->CallbackHandler == NULL){
		return;
	}
	// call callback handler for this task.
	task->CallbackHandler(task);
}
	
void task_scheduler_init(BOOLEAN statOn, uint32_t delay)
{
	taskScheduler.TaskIndex = -1;
	taskScheduler.Tick = 0;
	taskScheduler.Delay = delay;
	taskScheduler.Statistics = statOn;
}

volatile Task_t* task_scheduler_add(TaskJobFunc job, char *jobName, TaskRuleFunc rule, TaskPriority_t priority, int numOfExec, uint32_t period, uint32_t startupDelay, TaskCallbackHandler callback, BOOLEAN statDisable)
{
	taskScheduler.TaskIndex++;
	// if index ewxceeded the size of task queue, 
	// return null pointer
	if(taskScheduler.TaskIndex >= NUM_OF_TASK_QUEUE){
		return NULL;
	}
	
	volatile Task_t *t = &taskScheduler.Task[taskScheduler.TaskIndex];
	t->ExecutionTime = 0;
	t->ExecutionCount = 0;
	t->NumOfExecution = numOfExec;
	t->BeginTime = 0;
	t->EndTime = 0;
	t->Index = taskScheduler.TaskIndex;
	t->Job = job;
	t->JobName = jobName;
	t->Rule = rule;
	t->Period = period;
	t->StartupDelay = startupDelay;
	t->Priority = priority;
	t->Status = TS_IDLE;
	t->Suspend = TRUE;
	t->CallbackHandler = callback;
	t->Statistics = !statDisable;
	t->TimeCorrection = TRUE;
	return t;
}

void task_start(volatile Task_t *task)
{
	if(task->ExecutionCount == 0 && task->StartupDelay != 0)
		task->NextScheduleTime = GET_TICK() + task->StartupDelay + task->Period;
	else
		task->NextScheduleTime = GET_TICK() + task->Period;
	task->Status = TS_READY;
	task->Suspend = FALSE;
}

void task_restart(volatile Task_t *task)
{
	task->ExecutionCount = 0;
	task_start(task);
}


void task_start_all(void)
{
	volatile Task_t *task;
	for(int i=0; i<taskScheduler.TaskIndex+1; i++){
		task = &taskScheduler.Task[i];
		if(task->Status == TS_KILLED)
			continue;
		task_start(task);
	}
}

void task_start_immediately(volatile Task_t *task)
{
	task->Suspend = FALSE;
	task_execute(task);
}

void task_suspend(volatile Task_t *task)
{
	task->NextScheduleTime = GET_TICK() + task->Period;
	task->Status = TS_SUSPENDED;
	task->Suspend = TRUE;
}

void task_kill(volatile Task_t *task)
{
	task->Status = TS_KILLED;
	task->Suspend = FALSE;
	task_fire_event(task);
}


void task_execute(volatile Task_t *task)
{
	int timecorrection = 0;
	task->BeginTime = GET_TICK();

	if(task->TimeCorrection){
		if(task->NextScheduleTime > 0){
			timecorrection = (task->BeginTime - task->NextScheduleTime);
			if(timecorrection > task->Period){
				if(task->Period > 0)
					timecorrection = task->Period-1;
				else
					timecorrection = 0;
			}
		}
	}
	
	task->ReturnCode = 0;
	if(task->Suspend){
		task->EndTime = GET_TICK();
		task->BeginExecTime = task->BeginTime;
		task->ExecutionExecTime = task->ExecutionTime = (task->EndTime - task->BeginTime);
		task->NextScheduleTime = task->EndTime + task->Period;
		// if status different fire event.
		if(task->Status != TS_SUSPENDED){
			task_fire_event(task);
		}
		return;
	}
	
	if(task->NumOfExecution > 0 && task->ExecutionCount >= task->NumOfExecution){
		task->Status = TS_EXEC_COUNT;
		task->EndTime = GET_TICK();
		task->BeginExecTime = task->BeginTime;
		task->ExecutionExecTime = task->ExecutionTime = (task->EndTime - task->BeginTime);
		task->NextScheduleTime = task->EndTime + task->Period - timecorrection;
		task_fire_event(task);
		return;
	}
	
	if(task->Rule != NULL){
		task->Status = TS_RULE;
		if(task->Rule(task)){
			task->EndTime = GET_TICK();
			task->BeginExecTime = task->BeginTime;
			task->ExecutionExecTime = task->ExecutionTime = (task->EndTime - task->BeginTime);
			task->NextScheduleTime = task->EndTime + task->Period - timecorrection;
			task_fire_event(task);
			return;
		}
	}
	
	task->Status = TS_READY;
	task_fire_event(task);
	// call job function
	if(task->Job != NULL){
		task->Status = TS_RUNNING;
		task_fire_event(task);
		task->BeginExecTime = GET_TICK();
		task->ReturnCode = task->Job(task);
	}
	task->EndTime = GET_TICK();
	task->NextScheduleTime = task->EndTime + task->Period - timecorrection;
	task->ExecutionTime = (task->EndTime - task->BeginTime);
	task->ExecutionExecTime = (task->EndTime - task->BeginExecTime);
	task->ExecutionCount++;
	task->Status = TS_FINISHED;
	task_fire_event(task);
}

void task_scheduler_stop(void)
{
	startFlag = FALSE;
}

void task_scheduler_start(void)
{
	volatile uint32_t tick;
	volatile Task_t *task, *taskExec;
	startFlag = TRUE;
	while(startFlag){
		taskExec = NULL;
		taskScheduler.Tick = GET_TICK();
		tick = taskScheduler.Tick;
		for(int i=0; i<taskScheduler.TaskIndex+1; i++){
				task = &taskScheduler.Task[i];
				if(task->Status != TS_KILLED && tick >= task->NextScheduleTime){
					tick = task->NextScheduleTime;
					taskExec = task;
				}
		}
		if(taskExec == NULL){
			continue;
		}
		if(taskExec->Priority != TP_HIGHEST){
			for(int i=0; i<taskScheduler.TaskIndex+1; i++){
				task = &taskScheduler.Task[i];
				if(task->Status != TS_KILLED && 
					 taskExec->NextScheduleTime == task->NextScheduleTime){
					if(task->Priority < taskExec->Priority){
						taskExec = task;
					}
				}
			}
		}
		task_execute(taskExec);
		if(taskScheduler.Statistics && taskExec->Statistics){
			task_stats(taskExec);
		}
		if(taskExec->Status >= TS_RUNNING){
			TSCH_DELAY(taskScheduler.Delay);
		}
	}
}
