#!/bin/sh
#
##
## Copyright (C) 1996-2020 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##
#  Run all or some build tests for a given OS environment.
#
top=`dirname $0`

globalResult=0

action=""
cleanup="no"
verbose="no"
keepGoing="no"
remove_cache_file="true"
while [ $# -ge 1 ]; do
    case "$1" in
    --action)
	shift
	action="$1"
	shift
	;;
    --cleanup)
	cleanup="yes"
	shift
	;;
    --verbose)
	verbose="yes"
	shift
	;;
    --builddir)
    shift
    builddir="$1"
    shift
    ;;
    --keep-going)
	keepGoing="yes"
	shift
	;;
    --use-config-cache)
        #environment variable will be picked up by buildtest.sh
        cache_file=/tmp/config.cache.$$
        export cache_file
        shift
        ;;
    --aggressively-use-config-cache)
        #environment variable will be picked up by buildtest.sh
        #note: use ONLY if you know what you're doing
        cache_file=/tmp/config.cache
        remove_cache_file="false"
        export cache_file
        shift
        ;;
    *)
    	break
	;;
    esac
done

logtee() {
    if [ $verbose = yes ]; then
	tee $1
    else
	cat >$1
    fi
}

buildtest() {
    opts=$1
    action=$2
    layer=`basename ${opts} .opts`
    builddir="${builddir:-bt${layer}}"
    log=bt${layer}.log
    echo "TESTING: ${layer}"
    if test -e ${builddir}; then
	chmod -R 777 ${builddir};
    fi
    rm -f -r ${builddir} || ( echo "FATAL: Failed to prepare test build sandpit." ; exit 1 )
    mkdir ${builddir}
    if test "${verbose}" = "yes" ; then
        ls -la ${builddir}
    fi
    {
	result=255
	cd ${builddir}
	if test -e $top/test-suite/buildtest.sh ; then
	    $top/test-suite/buildtest.sh "${action}" ${opts} 2>&1
	    result=$?
	elif test -e ../$top/test-suite/buildtest.sh ; then
	    ../$top/test-suite/buildtest.sh "${action}" ../${opts} 2>&1
	    result=$?
	else
	    echo "Error: cannot find $top/test-suite/buildtest.sh script"
	    result=1
	fi

	# log the result for the outer script to notice
	echo "buildtest.sh result is $result";
    } 2>&1 | logtee ${log}

    result=1 # failure by default
    if grep -q '^buildtest.sh result is 0$' ${log}; then
	result=0
    fi

    # Display BUILD parameters to double check that we are building the
    # with the right parameters. TODO: Make less noisy.
    grep -E "BUILD" ${log}

    errors="^ERROR|\ error:|\ Error\ |No\ such|assertion\ failed|FAIL:|:\ undefined"
    grep -E "${errors}" ${log}

    if test $result -eq 0; then
	# successful execution
	if test "${verbose}" = "yes"; then
	    echo "Build OK. Global result is $globalResult."
	fi
	if test "${cleanup}" = "yes" ; then
	    echo "REMOVE DATA: ${builddir}"
	    chmod -R 777 ${builddir}
	    rm -f -r ${builddir}
	    echo "REMOVE LOG: ${log}"
	    rm -f -r ${log}
	fi
    else
        if test "${verbose}" != "yes" ; then
            echo "Build Failed. Last log lines are:"
            tail -20 ${log}
        else
            echo "Build FAILED."
        fi
        globalResult=1
    fi
}

# if using cache, make sure to clear it up first
if [ -n "$cache_file" -a -e "$cache_file" -a "$remove_cache_file" = "true" ]; then
    rm $cache_file
fi

if [ -f "$top/configure" -a -f "$top/libltdl/configure" ]; then
    echo "Already bootstrapped, skipping step"
else
    (cd "$top"; ./bootstrap.sh)
fi

# Decide what tests to run, $* contains test spec names or filenames.
# Use all knows specs if $* is empty or a special macro called 'all'.
if test -n "$*" -a "$*" != all; then
    tests="$*"
else
    tests=`ls -1 $top/test-suite/buildtests/layer*.opts`
fi

for t in $tests; do
    if test -e "$t"; then 
	# A configuration file
        cfg="$t"
    elif test -e "$top/test-suite/buildtests/${t}.opts"; then
	# A well-known configuration name
	cfg="$top/test-suite/buildtests/${t}.opts"
    else
	echo "Error: Unknown test specs '$t'"
	cfg=''
	globalResult=1
    fi

    # run the test, if any
    if test -n "$cfg"; then
	buildtest "$cfg" "$action"
    fi

    # quit on errors unless we should $keepGoing
    if test $globalResult -ne 0 -a $keepGoing != yes; then
	exit $globalResult
    fi
done

# if using cache, make sure to clear it up first
if [ -n "$cache_file" -a -e "$cache_file" -a "$remove_cache_file" = "true" ]; then
    rm $cache_file
fi

exit $globalResult
