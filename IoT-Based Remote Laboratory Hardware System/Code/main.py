import BlynkLib
import time 
import RPi.GPIO as GPIO
from datetime import datetime as dt
import requests
import logging
import logging.handlers

# Create a rotating file handler with a maximum of 1000 lines and 1 backup file
handler = logging.handlers.RotatingFileHandler(
    filename='/home/pi/Desktop/RTL/app.log', maxBytes=999999999, backupCount=1)
handler.setLevel(logging.DEBUG)
handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))

# Set up the root logger with the rotating file handler
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
logger.addHandler(handler)

logger.info("\n----------------------\n")
logger.info("Error log started")
#set GPIO pinout
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

# pulse width
delay = 0.0001


# GPIO variables setup
obj_dir = 2
obj_step = 3
obj_enab = 27
scr_dir = 23
scr_step = 24
scr_enab = 22
obj_ls = 5
scr_ls = 6
f_pin = 12
usage = 0

pins = (obj_dir,obj_step,obj_enab,scr_dir,scr_enab,scr_step,f_pin)

for pin in pins:
    GPIO.setup(pin,GPIO.OUT)

GPIO.setup(obj_ls,GPIO.IN)
GPIO.setup(scr_ls,GPIO.IN)

start_time = dt.now()
# Numeric Variables 
curpos_obj = 7.6
curpos_scr = 6.5
newpos_obj = 0.0
newpos_scr = 0.0
diff_scr = 0.0
diff_obj = 0.0

# Video Keys 
# Blynk Cloud server

BLYNK_TEMPLATE_ID = 'TMPLhtRbLfx4'
BLYNK_DEVICE_NAME = 'Led'
BLYNK_AUTH_tok = 'LvC6vyL_uPSpPdgnlCln0I9Vab6zcogV'

blynk = BlynkLib.Blynk(BLYNK_AUTH_tok,heartbeat=35)

# # Create BlynkTimer Instance
# timer = BlynkTimer.BlynkTimer()

def init():
    
  # set output state of output pins
  GPIO.output(obj_step,GPIO.LOW)
  GPIO.output(obj_dir,GPIO.LOW)
  GPIO.output(obj_enab,GPIO.HIGH)
  GPIO.output(scr_step,GPIO.LOW)
  GPIO.output(scr_dir,GPIO.LOW)
  GPIO.output(scr_enab,GPIO.HIGH)
  GPIO.output(f_pin,GPIO.LOW)
  #print("Recal")
  logger.info("Recal")
  obj_recal()
  time.sleep(1)
  scr_recal()
  time.sleep(1)
  logger.info("Recal done")
  #print("Recal done")
  blynk.virtual_write(6,0)
  global usage
  usage = 0
  blynk.virtual_write(0,1)
  blynk.sync_virtual(0)
  

  

# Virtual Pins 
# V0 is For Reclabiration
# V1 is controlling Screen 
# V2 is controlling Object
# V6 is for usage
def obj_recal():
  global curpos_obj
  GPIO.output(obj_dir,GPIO.HIGH)
  GPIO.output(obj_enab,GPIO.LOW)
  while(GPIO.input(obj_ls)==0):
    GPIO.output(obj_step,GPIO.HIGH)
    time.sleep(delay)
    GPIO.output(obj_step,GPIO.LOW)
    time.sleep(delay)
  GPIO.output(obj_enab,GPIO.HIGH)
  return 1

def scr_recal():
  global curpos_scr
  GPIO.output(scr_dir,GPIO.LOW)
  GPIO.output(scr_enab,GPIO.LOW)
  curpos_scr = 6.5
  while(GPIO.input(scr_ls)==0):
    GPIO.output(scr_step,GPIO.HIGH)
    time.sleep(delay)
    GPIO.output(scr_step,GPIO.LOW)
    time.sleep(delay)
  GPIO.output(scr_enab,GPIO.HIGH)
  return 1


def move_obj(value):
  global newpos_obj
  global curpos_obj
  global diff_obj

  newpos_obj = float(value)
  diff_obj = newpos_obj - curpos_obj
  curpos_obj = newpos_obj

  if(diff_obj < 0.0):
    GPIO.output(obj_dir,GPIO.HIGH)
  else:
    GPIO.output(obj_dir,GPIO.LOW)
    
  
  steps_obj = int(diff_obj*1000)
  
  GPIO.output(obj_enab,GPIO.LOW)
  for i in range(0,abs(steps_obj)):
    GPIO.output(obj_step,GPIO.HIGH)
    time.sleep(delay)
    GPIO.output(obj_step,GPIO.LOW)
    time.sleep(delay)

  GPIO.output(obj_enab,GPIO.HIGH)
  # curpos_obj = curpos_obj + diff_obj

def move_scr(value):
  global newpos_scr
  global curpos_scr
  global diff_scr

  newpos_scr = float(value)
  diff_scr = newpos_scr - curpos_scr
  curpos_scr = newpos_scr
  
  if(diff_scr < 0.0):
    GPIO.output(scr_dir,GPIO.LOW)
  else:
    GPIO.output(scr_dir,GPIO.HIGH)
    
  steps_scr = int(diff_scr*1000)
  GPIO.output(scr_enab,GPIO.LOW)

  for i in range(0,abs(steps_scr)):
    GPIO.output(scr_step,GPIO.HIGH)
    time.sleep(delay)
    GPIO.output(scr_step,GPIO.LOW)
    time.sleep(delay)
  
  GPIO.output(scr_enab,GPIO.HIGH)
    



@blynk.on('V0')
def S1_write_handler(value):
  global curpos_scr
  global curpos_obj
  if int(value[0]) == 1:
    #print('Recall Blynk')
    logger.info("Recall Blynk")
    obj_val = obj_recal()
    curpos_obj = 7.6
    scr_val = scr_recal()
    curpos_scr = 6.5
    if(obj_val == 1 & scr_val == 1):
        blynk.virtual_write(1,20)
        blynk.virtual_write(2,20)
        blynk.sync_virtual(1)
        blynk.sync_virtual(2)
        blynk.virtual_write(0,0)
    #print('Recal Done')
    logger.info("Recal Done")
  
@blynk.on('V6')    
def usage_handler(value):
  global start_time
  global usage
  if(value[0] == '1'):
      start_time = dt.now()
      GPIO.output(f_pin,GPIO.HIGH)
      usage = 1
  if(value[0] == '0'):
      GPIO.output(f_pin,GPIO.LOW)
      usage = 0
     
@blynk.on('V2')
def S1_write_handler(value):
    #print('scr_val=')
    #print(value[0])
    logger.info("Screen_val="+str(value[0]))
    move_scr(value[0])

    
@blynk.on('V1')
def S1_write_handler(value):
    #print('obj_val=')
    #print(value[0])
    logger.info("Object_val="+str(value[0]))
    move_obj(value[0])
    


@blynk.on("connected")
def blynk_connected():
  #print("Connection Success!!")
  logger.info("Connected to Blynk cloud...")
  blynk.sync_virtual(1)
  blynk.sync_virtual(2)
  blynk.sync_virtual(0)
  
  
@blynk.on("disconnected")
def blynk_disconnect():
    logger.warning("Blynk Disconnected, Retrying again...")
    try:        
        blynk.connect()
    except:
        time.sleep(10)
        blynk_disconnect()
    else:
        pass




if __name__ == "__main__":
    #blynk.run()
    init() #runs once
    while True:
        blynk.run()

