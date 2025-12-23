#!/usr/bin/env python3

from http.server import HTTPServer, SimpleHTTPRequestHandler, test, ThreadingHTTPServer
import sys, os

PROJECT_ROOT = os.path.abspath(os.path.join(os.getcwd(), "../../"))

class Server(SimpleHTTPRequestHandler):
  def end_headers(self):
    # Allow DevTools to load source files
    self.send_header('Access-Control-Allow-Origin', '*')
    self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')    

    self.send_header('Cross-Origin-Resource-Policy', 'cross-origin')
    self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
    self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')

    # Prevent caching to ensure you always see the latest changes
    self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')

    SimpleHTTPRequestHandler.end_headers(self)

  def translate_path(self, path):
    original_path = super().translate_path(path)
    if os.path.exists(original_path):
      return original_path    

    rel_path = os.path.relpath(original_path, os.getcwd())
    alt_path = os.path.join(PROJECT_ROOT, rel_path)
    if os.path.exists(alt_path):
      return alt_path    
    return original_path
    
  def handle_one_request(self):
    # print(f"DEBUG: Request received for path: {self.path}")
    try:
      super().handle_one_request()
    except Exception:
      pass    

if __name__ == '__main__':
  # Ensure we are serving from the directory where the script is located
  # allowing you to run the script from anywhere
  os.chdir(os.path.dirname(os.path.abspath(__file__)))  

  port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
  print(f"Serving ToyOS from: {os.getcwd()}")
  print(f"Source Root Mapped to: {PROJECT_ROOT}")
  print(f"URL: http://localhost:{port}")
  
  test(Server, ThreadingHTTPServer, port=port)