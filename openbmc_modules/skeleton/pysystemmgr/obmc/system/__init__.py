from os.path import join
from glob import glob


def find_gpio_base(path="/sys/class/gpio/"):
    pattern = "gpiochip*"
    for gc in glob(join(path, pattern)):
        with open(join(gc, "label")) as f:
            label = f.readline().strip()
        if label == "1e780000.gpio":
            with open(join(gc, "base")) as f:
                return int(f.readline().strip())
    # trigger a file not found exception
    open(join(path, "gpiochip"))


GPIO_BASE = find_gpio_base()


def convertGpio(name):
    offset = int(''.join(list(filter(str.isdigit, name))))
    port = list(filter(str.isalpha, name.upper()))
    a = ord(port[-1]) - ord('A')
    if len(port) > 1:
        a += 26
    base = a * 8 + GPIO_BASE
    return base + offset
