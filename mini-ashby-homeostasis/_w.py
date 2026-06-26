import os
B = r"F:\nano-everything\mini-complex-control-theory\29. mini-cybernetics-foundation\mini-ashby-homeostasis"
def w(r,c):
  p=os.path.join(B,r)
  with open(p,"w",encoding="utf-8") as f: f.write(c)
  print("OK",r,len(c))
