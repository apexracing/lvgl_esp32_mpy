

from hardware import display

import lvgl as lv
import lvgl_esp32
import os
wrapper = lvgl_esp32.Wrapper(display)
wrapper.init()
display.brightness(100)
#顺时转180度
display.mirrorX(True)
display.mirrorY(True)
screen = lv.screen_active()
screen.set_style_bg_color(lv.color_hex(0xFF0000), lv.PART.MAIN | lv.STATE.DEFAULT)
screen.set_style_bg_opa(255, lv.PART.MAIN | lv.STATE.DEFAULT)
screen.set_style_bg_grad_color(lv.color_hex(0x000000), lv.PART.MAIN | lv.STATE.DEFAULT)
screen.set_style_bg_main_stop(0, lv.PART.MAIN | lv.STATE.DEFAULT)
screen.set_style_bg_grad_stop(255, lv.PART.MAIN | lv.STATE.DEFAULT)
screen.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.DEFAULT)

label = lv.label(screen)
label.set_text(f"MicroPython{os.uname()[2]}")
label.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN)
label.align(lv.ALIGN.CENTER, 0, 0)

lv_ver = lv.label(screen)
lv_ver.set_text(f"LVGL {lv.version_major()}.{lv.version_minor()}.{lv.version_patch()}")
lv_ver.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN)
lv_ver.align_to(label,lv.ALIGN.BOTTOM_MID, 0, -10)

a = lv.anim_t()
a.init()
a.set_var(label)
a.set_values(10, 50)
a.set_duration(1000)
a.set_playback_delay(100)
a.set_playback_duration(300)
a.set_repeat_delay(500)
a.set_repeat_count(lv.ANIM_REPEAT_INFINITE)
a.set_path_cb(lv.anim_t.path_ease_in_out)
a.set_custom_exec_cb(lambda _, v: label.set_y(v))
a.start()
print("启动")
lv.timer_handler_run_in_period(5)
lv.timer_handler_run_in_period(5)



