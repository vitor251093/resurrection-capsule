
def get(request, key, type):
    if type == int:
        try:
            return int(request.args.get(key))
        except ValueError:
            return None
    if type == str:
        return request.args.get(key)
    if type == bool:
        return (request.args.get(key, default='') == 'true')
    return None