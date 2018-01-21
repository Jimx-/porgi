R"(
def _route(self, rule, methods=["GET"]):
    def decorator(f):
        self.register_route(rule, f, methods)
        return f
    return decorator
Porgi.route = _route
Porgi.make_response = _HttpResponse
)"
