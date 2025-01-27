#pragma once
/**
	\enum TaskState
	\brief State of the task
	\note RUNNING makes the task available for scheduling
	\note A TERMINATED task will be reaped in the next timer interrupt
*/
enum class TaskState {
	RUNNING,
	WAITING,
	SLEEPING,
	TERMINATED
};