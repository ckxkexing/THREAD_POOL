import threading
import time


class MyBlockingQueue:
    def __init__(self, capability: int):
        self.capability = capability
        self.lock = threading.Lock()
        self.notFull = threading.Condition(self.lock)
        self.notEmpty = threading.Condition(self.lock)

        self.list = []

    def put(self, obj):
        with self.lock:

            while(len(self.list) == self.capability):
                self.notFull.wait()

            self.list.append(obj)

            self.notEmpty.notify()


    def get(self, timeout = None):
        with self.lock:
            end_time = time.time() + timeout if timeout is not None else None
            while(len(self.list) == 0):
                if timeout is not None:
                    remaining_time = end_time - time.time()
                    if remaining_time < 0:
                        return None
                    self.notEmpty.wait(remaining_time)
                else:
                    self.notEmpty.wait()

            obj = self.list.pop(0)
            self.notFull.notify()

            return obj
    
    def size(self):
        with self.lock:

            size = len(self.list)

            return size

# 使用示例
my_queue = MyBlockingQueue(10)

# 在另一个线程中 put 一个元素
def put_item():
    time.sleep(2)
    my_queue.put("Hello from another thread")

threading.Thread(target=put_item).start()

# 在主线程中通过 get 获取元素，设置超时为 1 秒
result = my_queue.get(timeout=1)
print(result)
result = my_queue.get(timeout=1)
print(result)
result = my_queue.get(timeout=1)
print(result)
