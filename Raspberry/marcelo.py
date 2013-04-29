import requests
import serial

class PostSerialData():
	def __init__(self, usb_port, baud_rate,  url):
		self.usb_port = usb_port
		self.baud_rate = baud_rate
		self.url = url
		self.ser = serial.Serial("/dev/" + usb_port , baudrate=self.baud_rate, timeout=1)

	def post_location(self, data): 
		rfid, lati, longi = data.split(":")[1][:data.split(":")[1].index(";")].split(",") # pega a string no formato "L:id,lat,longi;" e extrai os 3 dados dela
		print "Postando nova localizacao: id:" + rfid + ", lati:" + lati + ", longi:" + longi
		payload = {"rfid": rfid, "x": longi, "y": lati, "status": ""}
		req = requests.post(self.url, data=payload)
		if req.status_code == 200:
			print "Localizacao postada com sucesso\n"
		else:
			print "Problema no post"

	def start_posting(self):
		print "Aguardando dados..\n"
		while True:
			data = self.ser.readline()
			if len(data) == 0:
				pass
			elif data[0:2] == 'L:' and data.count(",") == 2 and (";" in data.split(",")[2]):
				print "Recebeu " + data
				self.post_location(data)		
			else:
				print "Comando invalido -> " + data

if __name__ == "__main__":
	teste = PostSerialData("ttyUSB0", 9600, "http://rcsr.p.ht/concretid/modulos/peca.update.hw.php")
	teste.start_posting() 