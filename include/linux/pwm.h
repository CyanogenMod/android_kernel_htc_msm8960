#ifndef __LINUX_PWM_H
#define __LINUX_PWM_H

struct pwm_device;


struct pwm_device __weak *pwm_request(int pwm_id, const char *label);

void __weak pwm_free(struct pwm_device *pwm);

int __weak pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns);

int __weak pwm_enable(struct pwm_device *pwm);

void __weak pwm_disable(struct pwm_device *pwm);

#endif 
