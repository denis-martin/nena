import os
from http.server import HTTPServer, BaseHTTPRequestHandler

port = 8080

netletFile = "/tmp/nodearch-available-netlets"
lossFile = "/tmp/nodearch-pkt-loss"

# httpd handler class
class httpdHandler(BaseHTTPRequestHandler):
	def do_GET(self):

		file = ""
		request = self.path[1:]
		if request == "nodearch-available-netlets":
			file = netletFile
		if request == "nodearch-pkt-loss":
			file = lossFile
		
		if os.path.exists(file):
			self.send_response(200)
			#self.send_header("Content-type", "text/html")
			self.send_header("Content-type", "text/plain")
			self.end_headers()
			with open(file) as f:
				for line in f:
					#response += line
					self.wfile.write(line.encode("UTF8"))
		else:
			self.send_response(404)
	

	def do_POST(self):

		# send response	
		self.send_response(204)
		self.send_header("Content-Length", "0")
		self.end_headers()
	
		# read input and write it to the corresponding file
		file = ""

		request = self.path[1:]
		if request == "nodearch-available-netlets":
			file = netletFile
		if request == "nodearch-pkt-loss":
			file = lossFile

		if file != "":
			f = open(file, "w")

			for line in self.rfile:
				f.write(line.decode("UTF8"))
			f.close


# run httpd
def run_httpd(server_class=HTTPServer, handler_class=BaseHTTPRequestHandler):
	try:
		print("Starting httpd...")
		server_address = ('', port)
		httpd = server_class(server_address, handler_class)
		httpd.serve_forever()
	except KeyboardInterrupt:
		print("Received keyboard interrupt. Shutting down...")
		httpd.server_close()


# main
if __name__ == "__main__":

	# run httpd
	run_httpd(handler_class=httpdHandler)

