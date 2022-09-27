#!/usr/bin/env python3

import datetime
import subprocess

def git_node_short(dirname):
    return subprocess.check_output(
        ['git', 'rev-parse', '--short=10', 'HEAD'],
        cwd=dirname).decode(encoding='utf-8').strip()

def git_describe(dirname):
    return subprocess.check_output(
        ['git', 'describe'],
        cwd=dirname).decode(encoding='utf-8').strip()

def git_date(dirname):
    timestamp = subprocess.check_output(
        ['git', 'log', '-1', '--format=format:%at'],
        cwd=dirname).decode(encoding='utf-8')
    return datetime.datetime.fromtimestamp(float(timestamp)).strftime('%B %d, %Y')

def date_and_version():
    date = git_date('.')
    try:
        version = git_describe('.')
    except:
        version = git_node_short('.')
    return r'\date{%s (%s)}' % (date, version)

if __name__ == '__main__':
    print(date_and_version())
