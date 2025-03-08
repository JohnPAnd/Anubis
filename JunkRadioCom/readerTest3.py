import sysv_ipc
import struct

from time import sleep
WAIT_TIME = 0.02

BANGBANG_ENABLE = False
PRESSURE_SET = 200

try:
	#Create a key
	MEMKEY_GL = 2222
	MEMKEY_DAC = 1111

	# Create shared memory object
	sharedMem_GL = sysv_ipc.SharedMemory(MEMKEY_GL, size=8)
	sharedMem_DAC = sysv_ipc.SharedMemory(MEMKEY_DAC, size=28)


	#memory_value = sharedMem_GL.read()
	#value = struct.unpack("<?xxxi", memory_value)
	#print(value)

	writeBytes = struct.pack("<?xxxi", BANGBANG_ENABLE, PRESSURE_SET)
	sharedMem_GL.write(writeBytes)

	while True:
		average = 0
		for i in range(10):
			memory_value = sharedMem_DAC.read()
			value = struct.unpack("<Iffffff", memory_value)
			average = average + value[1]
			#print(round(value[1],2))
			sleep(WAIT_TIME)
		average = average / 10
		print(round(average,2))

except Exception as exc:
	print("Failed")
	print(exc)
	pass

sharedMem_GL.detach()
sharedMem_DAC.detach()
