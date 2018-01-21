# Porgi

Porgi is a lightweight web server written in C++ with a Python interface.

## Requirements

Porgi can only be built on Linux 2.6+. To build Porgi, these components are required:
 * Python 3
 * Boost.Python
 * CMake 3.5+
 * [vit-vit/CTPL][1]
 * [muflihun/easyloggingpp][2]
 * [jarro2783/cxxopts][3]

On Ubuntu, these packages can be installed by:
```
apt-get install cmake libboost-python-dev python3 python3-dev
```

## Compile Porgi

```
git submodule init
git submodule update
mkdir build && cd build
cmake .. && make
```

## Usage

Porgi is very easy to use. A Python script is needed to tell Porgi how the requests should be handled.
```python
@porgi.route('/hello') # specify what URL pattern this handler is responsible for
def hello(request):
    # get information about the HTTP request
    print(request.uri)
    print(request.method)
    print(request.headers['Host'])

    # respond to the client
    return porgi.make_response(200, {'Content-Type': 'text/plain'}, 'Hello world!')

# now with a bit of pattern matching
@porgi.route('/hello/:name')
def hello_with_name(request, params):
    return 'Hello ' + str(params['name']) + '!' # returning a string also works
```
Run Porgi with this Python script:
```
porgi app.py
```
Now you can open your browser and visit `http://localhost:8080/hello` or `http://localhost:8080/hello/<your name>`. 

By default Porgi listens on port 8080. If you want to assign port manually, use the `-p <port>` option. Porgi supports multi-threading. The number of worker threads can be specified by the `-n <ncpus>` option.

  [1]: https://github.com/vit-vit/CTPL
  [2]: https://github.com/muflihun/easyloggingpp
  [3]: https://github.com/jarro2783/cxxopts

