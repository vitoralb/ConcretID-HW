import requests
import serial
import json

class PostSerialData():
	def __init__(self, usb_port, baud_rate,  url):
		self.usb_port = usb_port
		self.baud_rate = baud_rate
		self.url = url
		self.ser = serial.Serial("/dev/"+ usb_port , baudrate=self.baud_rate, timeout=1)
	
	def post_location(self, data):
		idt, alt, longi = data.split(":")[1][:-1].split(",") # pega a string no formato "L:id,lat,longi;" e extrai os 3 dados dela
		print "postando nova localizacao: " + idt + ", " + alt + ", " + longi
		payload = {'id': idt, 'altitude': alt, 'longitude': longi}
		req = requests.post(self.url, data=payload)

		print req.text
		
	def post_status(self, data):
		idt, status = data.split(":")[1][:1].split(",") # pega a string no formato "S:id,status;" e extrai os 2 dados dela
		print "postando novo status: " + idt + ", " + status
		payload = {'id': idt, 'status': status}
		req = requests.post(self.url, data=payload)
		print req.text	

	def get_info(self, data):
		idt = data[2:-1] #pega o unico dado em "I:id;"
		print "solicitando informacoes de: " + idt
		payload = {'id': idt}
		req = requests.get(self.url, data=payload)
		print req.content
		self.ser.write(req)
		

	def start_posting(self):
		while True:
			data = self.ser.readline()
			print "recebeu: " + data
			if len(data) == 0:
				print "aguardando novos dados"
			elif data[-1] != ';' or data[0:2] not in {'L:','S:','I:'}:
				print "formato invalido, operador invalido ou faltou ; no final"
			elif data[0:2] == 'L:' and data.count(",") == 2:
				self.post_location(data)
			elif data[0:2] == 'S:' and data.count(",") == 1:
				self.post_status(data)
			elif data[0:2] == 'I:':
				self.get_info(data)		
			else:
				print "formato invalido, provavelmente no numero de parametros"
			
if __name__ == "__main__":
	teste = PostSerialData("ttyUSB0", 9600, "http://posttestserver.com/post.php")
	teste.start_posting() 
