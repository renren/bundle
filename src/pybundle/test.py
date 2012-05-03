# coding=utf-8

import sys, os
import glob
import bundle
import datetime, time
import signal, threading
from optparse import OptionParser

class RepeatTimer(threading.Thread):
    def __init__(self, interval, function, iterations=0, args=[], kwargs={}):
        threading.Thread.__init__(self)
        self.interval = interval
        self.function = function
        self.iterations = iterations
        self.args = args
        self.kwargs = kwargs
        self.finished = threading.Event()
 
    def run(self):
        count = 0
        # hack for
        self.finished.wait(datetime.datetime.now().second % self.interval)
        while not self.finished.isSet() and (self.iterations == 0 or count < self.iterations):
            self.function(*self.args, **self.kwargs)
            count += 1
            if not self.finished.isSet():
                self.finished.wait(self.interval)
 
    def cancel(self):
        self.finished.set()

last_tm = None
last_count = 0
count = 0
last_byte_count = 0
byte_count = 0
latency = []
quit = False

def handler(signum, frame):
    global count, last_count, last_tm, byte_count, last_byte_count, latency
    now = datetime.datetime.now()
    # time, count, latency, bytes, pid
    if count - last_count > 0:
        t = float(sum(latency)) / (count - last_count)
    else:
        t = 0
    text = '%s \t%d \t%d \t%d \t%d\n' % (now.strftime('%X'), count - last_count, 
                                         t,
                                         byte_count - last_byte_count, os.getpid())
    os.write(sys.stdout.fileno(), text)
    last_tm = now
    last_count = count
    last_byte_count = byte_count
    latency = []

def process_write(folder, storage_folder):
    global count, latency, byte_count
    a = [x for x in glob.glob(os.path.join(folder, '*.jpg'))]
    global quit
    while not quit:
        for i in a:
            try:
                data = open(i, 'rb').read()

                beg = datetime.datetime.now()
                w= bundle.allocate(len(data), storage_folder)
                w.ensure_url(prefix='test/rubbish', postfix='.jpg')
                w.write(data)
                # print w.url
                latency.append((datetime.datetime.now() - beg).microseconds/1000)
                del w
                count += 1
                byte_count += len(data)
                if quit:
                    break
            except Exception,e:
                print e
                quit = True
                break

def process_read(listfile, storage_folder):
    global count, byte_count, latency, byte_count, quit
    for p in open(listfile):
        p = p.strip('\n')
        beg = datetime.datetime.now()
        d = bundle.read(storage_folder, p)
        latency.append((datetime.datetime.now() - beg).microseconds)
        count += 1
        byte_count += len(d)
        if quit:
            break

def sig_handler(signum, frame):
    global quit
    quit = True

if __name__ == '__main__':
    parser = OptionParser(usage="usage: %prog [options] folder")
    parser.add_option("-r", "--read", dest="read", help="read test", action='store_true')
    parser.add_option("-w", "--write", dest="write", help="write test", action='store_true')

    parser.add_option("-s", "--storage", dest="storage", help="storage folder for read and write")
    parser.add_option("-p", "--photo", dest="photo", help="photo folder for write")
    parser.add_option("-u", "--url-list", dest="url", help="url for read")
    (opts, args) = parser.parse_args()

    # python test.py -s ../../mfs/test -p ../../test/bundle -w
    # -s for mfs mount dir
    # -p for photo dir

    signal.signal(signal.SIGINT, sig_handler)


    r = RepeatTimer(1, handler, 0, args=[1,2])
    r.daemon = True

    if opts.write:
        if opts.storage is None or opts.photo is None:
            print 'need photo folder and storage'
            sys.exit()

        r.start()
        process_write(opts.photo, opts.storage)

    elif opts.read:
        if opts.storage is None or opts.url is None:
            print 'need url list and storage'
            sys.exit()
        r.start()
        process_read(opts.url, opts.storage)

    r.finished.set()
    if r.isAlive():
        r.join()