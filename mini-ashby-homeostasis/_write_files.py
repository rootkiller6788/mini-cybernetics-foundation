import os
BASE = r'F:\nano-everything\mini-complex-control-theory\29. mini-cybernetics-foundation\mini-ashby-homeostasis'
def write_file(relpath, content):
    with open(os.path.join(BASE, relpath), 'w', encoding='utf-8') as f:
        f.write(content)
    print('Wrote ' + relpath)
