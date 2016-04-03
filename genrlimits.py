import resource


def main():
    attribs = []
    for attrib in dir(resource):
        if attrib.startswith('RLIMIT_'):
            attribs.append((attrib, getattr(resource, attrib)))
    attribs.sort(key=lambda x: x[1])

    for k, v in attribs:
        print '  {"%s", %d},' % (k.rstrip('RLIMIT_'), v)


if __name__ == '__main__':
    main()
