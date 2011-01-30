#!/usr/bin/env python

import re
import sys

class TestCase(object):
    def __init__(self, function, plugins, desc):
        self.function = function
        self.plugins  = plugins
        self.desc     = desc

    def __str__(self):
        return 'TESTCASE(%s, %s, "%s")' % \
            (self.function, " | ".join(self.plugins), self.desc)


class Finder(object):
    def __init__(self):
        self.found = []
        self.regex = re.compile(
            "TESTCASE\\("
            "\\s*([a-zA-z_0-9]+)\\s*,"    # function name
            "\\s*([- |a-zA-z_0-9]+)\\s*," # plugins
            "\\s*\"([^\"]*)\"\\s*"        # description
            "\\)"
            , re.MULTILINE)

    def search_file(self, fname):
        f = open(fname, "r")
        src = ''.join(f)
        f.close()
        matches = self.regex.findall(src)
        del src

        for m in matches:
            name    = m[0]
            plugins = m[1].split('|')
            desc    = m[2]
            plugins = [p.strip() for p in plugins]
            self.found.append(TestCase(name, plugins, desc))


def search_files(args=sys.argv[1:]):
    f = Finder()
    for fname in args:
        try:
            f.search_file(fname)
        except IOError, msg:
            sys.stderr.write("%s: %s\n" % (fname, msg))
            sys.exit(1)
    return f.found


def create_tc_tables(out=sys.stdout):
    def _(o):
        out.write("%s\n" % str(o))

    tests   = search_files()

    plugins = set()
    for tc in tests:
        for p in tc.plugins:
            plugins.add(p)

    plugins.discard('ALL')
    plugins.discard('LITMUS')

    _('#include "tests.h"')

    for tc in tests:
        _('void test_%s(void);' % tc.function)

    _('struct testcase test_catalog[] = {')
    for tc in tests:
        _('\t{test_%s, "%s"},' % (tc.function, tc.desc))
    _('};')

    for p in plugins:
        count = 0
        _('int %s_TESTS[] = {' % p)
        for (i, tc) in enumerate(tests):
            if p in tc.plugins or \
                    'ALL' in tc.plugins or \
                    'LITMUS' in tc.plugins and p != 'LINUX':
                _('\t%d,' % i)
                count += 1
        _('};')
        _('#define NUM_%s_TESTS %d' % (p, count))

    _('struct testsuite testsuite[] = {')
    for p in plugins:
        _('\t{"%s", %s_TESTS, NUM_%s_TESTS},' % (p.replace('_', '-'), p, p))
    _('};')
    _('#define NUM_PLUGINS %s' % len(plugins))

if __name__ == '__main__':
    create_tc_tables()

