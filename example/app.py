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
