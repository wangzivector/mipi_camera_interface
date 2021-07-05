
#ifndef CAMERA_CONTROL_H_
#define CAMERA_CONTROL_H_

#include "parameter.h"

struct v4l2_queryctrl queryctrl;
struct v4l2_querymenu querymenu;
struct v4l2_control control;
struct v4l2_query_ext_ctrl query_ext_ctrl;


void enumerateMenu(unsigned long id);
void enumerateMenuList(void);
void enumerateExtMenuList(void);
void cameraFunctionsControlExample(void);
int cameraFunctionsControl(unsigned int fun_id, signed int fun_value);
void enumerateExtendedControl();


#endif /* PARAMETER_H_ */