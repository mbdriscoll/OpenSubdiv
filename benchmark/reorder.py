#!/usr/bin/env python

import sys, os, shelve

from bench import *

def build_db(model):
    db = set()
    for k in ["CPU", "OpenMP"]:
        for r in [0,1]:
            for l in range( modelMaxLevel[model] ):
                try:
                    run = do_run(frames=1000, model=model, kernel=k, level=l+1, reorder=r)
                    db.add(run)
                except ExecutionError as e:
                    print "\tFailed with: %s" % e.message
    return db

def gen_dat_file(ofile, db):
    kernel_set = set([ r.kernel for r in db if r.kernel ])
    size_set = set([ r.nverts for r in db if r.nverts ])
    kernel_list = sorted(kernel_set, key=lambda k: kernelNum[k])
    size_list = sorted(size_set)
    print >>ofile, "nVerts", " ".join(["%s %s-opt" % (name, name) for name in kernel_list])
    for size in size_list:
        print >>ofile, size,
        for kernel in kernel_list:
            run_list_reg = filter(lambda r: r.nverts == size and r.kernel == kernel and r.reorder == 0, db)
            run_list_opt = filter(lambda r: r.nverts == size and r.kernel == kernel and r.reorder == 1, db)
            if len(run_list_reg) == 1:
                print >>ofile, " %f %f" % (run_list_reg[0].mean(), run_list_opt[0].mean()),
            elif len(run_list_reg) == 0:
                print >>ofile, " ? ?",
        print >>ofile


def main(argv):
    model = argv[1][8:-4]
    with open("reorder_%s.dat" % model, 'w') as ofile:
        gen_dat_file(ofile, build_db(model))


if __name__ == '__main__':
    main(sys.argv)
