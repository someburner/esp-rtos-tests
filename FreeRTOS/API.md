# FreeRTOS API Reference

Simple reference for varous APIs.

## xTaskCreate

Create a new task and add it to the list of tasks that are ready to run. (Heap)

**Prototype**:

```
BaseType_t xTaskCreate(
   TaskFunction_t pvTaskCode, const char * const pcName,
   unsigned short usStackDepth, void *pvParameters,
   UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask
);
```
**Parameters**:

* *pvTaskCode* - Pointer to the task entry function
* *pcName* - A descriptive name for the task. Mainly for debugging.
* *usStackDepth* - The number of words (not bytes!) to allocate for use as the
  task's stack.
* *pvParameters* - A value that will passed into the created task as the task's
  parameter.
* *uxPriority* - The priority at which the created task will execute.
* *pxCreatedTask* - Used to pass a handle to the created task out of the
  xTaskCreate() function. pxCreatedTask is optional and can be set to NULL.























### end
