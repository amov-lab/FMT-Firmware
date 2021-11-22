# Build Lists
# Modify this file to control which files/modules should be built

DRIVERS = [
]

DRIVERS_CPPPATH = []

HAL = [
    # 'pin/*.c',
    'serial/*.c',
    'systick/*.c',
    # 'usb/usbd_cdc.c'
]

HAL_CPPPATH = []

MODULES = [
    'console/*.c',
    'system/*.c',
    'ipc/*.c',
    'utils/*.c',
    'toml/*.c',
    'workqueue/*.c',
    'math/*.c',
    'task_manager/*.c',
    'file_manager/*.c'
]

MODULES_CPPPATH = [
]

TASKS = [
]

TASKS_CPPPATH = []

LIBS = [
    'mavlink',
]
