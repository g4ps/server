import http.client

h1 = http.client.HTTPConnection('ev-gen.ru:80')
h1.connect()
h1.request("GET", "/", {"Host": "ev-gen.ru"}, "")
