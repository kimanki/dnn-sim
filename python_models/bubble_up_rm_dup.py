#!/usr/bin/python
# This script processes a csv of filters for one layer in Caffe
# this csv is provided by Jorge

import numpy as np
import sys
import math

import read_filters
import chunk

def printn (str):
    sys.stdout.write(str)

def print_weights(w):
    for r in range(0,w.shape[0]):
        printn( "%2d|" % r )
        for n in range (0,w.shape[1]):
            for i in range(0,w.shape[2]):
                printn( "%s" % w[r,n,i] )
            printn ("|")
        printn ("\n")

def print_filter(w,n):
    for r in range(0,w.shape[0]):
        printn( "%2d|" % r )
        for i in range(0,w.shape[2]):
            printn( "%s" % w[r,n,i] )
        printn ("|")
        printn ("\n")

def print_row(w,r):
    for n in range (0,w.shape[1]):
        for i in range(0,w.shape[2]):
            printn( "%s" % w[r,n,i] )
        printn ("|")
    printn ("\n")

def is_zero(w):
    return not w.any()

def zero():
    return 0

# for character arrays
# def is_zero(w):
    # for i in np.nditer(w):
        # if (i != '0' and i != 'E'):
            # return 0
    # return 1

# def zero():
    # return '0'

def look_for_replacement(r, n, i, weights, ind, lookaside, lookahead):

    # get dimensions 
    (R,Tn,Ti) = weights.shape

    # where to look for
    rmax = min(r + lookahead , R-1 )
    imin = max(0,i-lookaside)
    imax = min(Ti-1,i+lookaside)

    # lookaside
    for ri in range( imin, imax + 1):
        # lookahead
        for rr in range( r + 1 , rmax + 1 ):
            if (not is_zero(weights[rr,n,ri])):
                # found a replacement
                #print "Re: ", (r,n,i,weights[r,n,i]), (rr,n,ri,weights[rr,n,ri]) 
                weights[r,n,i] = weights[rr,n,ri]
                weights[rr,n,ri] = zero()
                ind[r,n,i] = ind[rr,n,ri]
                ind[rr,n,ri] = -3
                return (weights, ind)

    return (weights, ind)

def look_for_duplicates(r, n, i, weights, ind, lookahead):

    # get dimensions 
    (R,Tn,Ti) = weights.shape

    # provider's real index and weight
    (pr,pn,pi) = ind[r,n,i]
    pw = weights[r,n,i]

    # where to look for
    rmax = min(r + lookahead , R-1)
    look_in_n = set(range(0,Tn))
    look_in_n.remove(pn)

    dup_index = []

    for rr in range(r,rmax):

        remove_n = set()
        for nn in look_in_n:

            for ii in range(0,Ti):

                # get target's real index
                (dr,dn,di) = ind[rr,nn,ii]

                # found a dup if both the real r, i and weights are the same
                if (weights[rr,nn,ii] == pw and dr == pr and di == pi):
                   remove_n.add(nn)
                   dup_index.append((rr,nn,ii))
                   break

        look_in_n = look_in_n.difference(remove_n)

    return dup_index



def process_weights(weights, lookaside, lookahead):

    # gather stats about data
    # ones = np.count_nonzero(weights.count('1'))
    # print "ones  = ", ones
    # zeros = np.count_nonzero(weights.count('0')) 
    # print "zeros = ", zeros
    # percent = (ones + 0.0)/(ones+zeros)
    # print "percent ones = ", percent
    # rows = ( (ones + 0.0)/(ones+zeros) * 64 )
    # print "rows of ones = ", rows

    # for n in range(0,Tn):
        # col = weights[:,n,:]
        # ones = np.count_nonzero(col.count('1'))
        # zeros = np.count_nonzero(col.count('0')) 
        # rows = ( (ones + 0.0)/(ones+zeros) * 64 )
        # print n, "rows of ones = ", rows

    # print_weights(weights)
    # print_filter(weights,n)

    zero_rows = 0;
    out_limit = 1;
    in_limit = 1;

    (R,Tn,Ti) = weights.shape
    ind = np.indices((R,Tn,Ti)).swapaxes(0,3).swapaxes(0,2).swapaxes(0,1)

    print "R=",R
    for r in range(0,R-1):
    #    print "C:", weights[r,n,:]
    #    print "N:", weights[r+1,n,:]
        rmax = min(r + lookahead , R-1 )

        # print r, "##############################"
        # for tr in range(r, rmax + 1):
            # print_row(weights,tr)

        # check for all zeros
        if (is_zero( weights[r,:,:] ) ):
            # print r # print all lines that are all zeroes
            zero_rows += 1
            continue

        # counter for the limits
        in_ctr = [in_limit] * Tn
        out_ctr = [out_limit] * Ti

        # fill bubbles
        for n in range(0,Tn):
            for i in range(0,Ti):

                # fill in the bubble
                if (is_zero( weights[r,n,i] )):
                    # found a zero to fill, look for replacement
                    (weights, ind) = look_for_replacement(
                                r, n, i, weights, ind, lookaside, lookahead)

                # reached output limit for this cycle
                # or can't fill in the bubble
                if (not out_ctr[n] or is_zero( weights[r,n,i] )):
                   continue

                #print (r,n,i), ": ", (ind[r,n,i],weights[r,n,i])

                # look for duplicates of this value
                dup_index = look_for_duplicates(r, n, i, weights, ind, lookahead)

                # go through each of them and see if they will accept inputs 
                output_dup = False
                for dup in dup_index:
                    (dr,dn,di) = dup

                    # reached intput limit
                    if (not in_ctr[dn]):
                       continue

                    #print "Du: ", (ind[r,n,i],weights[r,n,i]), (ind[dr,dn,di],weights[dr,dn,di]) 
                    weights[dr,dn,di] = zero()
                    ind[dr,dn,di] = -2
                    output_dup = True
                    in_ctr[dn] += 1

                    # fill in the bubble if they are on the same row
                    # this loop already passed it
                    if (dr == r and dn < n):
                       (weights, ind) = look_for_replacement(
                                   dr, dn, di, weights, ind, lookaside, lookahead)

                if (output_dup):
                   out_ctr[n] += 1
                           
        # print "--------------------------------"
        # for tr in range(r, rmax + 1):
            # print_row(weights,tr)

    # print_filter(weights,n)
    # print_weights(weights)

    # check if the last row is zero
    if (is_zero( weights[R-1,:,:] ) ):
        zero_rows += 1

    print "row reduction = ", R-zero_rows , "/", R

    # print weights.any(axis=(1,2)) # print out false if a row is all zero

    # remove zero rows
    ind = ind[weights.any(axis=(1,2)),:,:]
    weights = weights[weights.any(axis=(1,2)),:,:]

    return (R-zero_rows,ind,weights)

######### MAIN ################################################################

def main():
    script, filename, lookaside= sys.argv
    lookaside = int(lookaside)
    lookahead = 3

    print "read filter file"
    # w is an Nn x Ni ndarray of weights
    w = read_filters.read_filters(filename)

    print "break into chunks"
    # chunks is a list of Nrows * Tn * Ti weights
    chunks = chunk.chunk(w)

    print "processing each chunk"
    for c in chunks:
        process_weights(c, lookaside, lookahead)
        #sys.exit()

if __name__ == "__main__":
    main()

