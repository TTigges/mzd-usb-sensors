#!/usr/bin/perl -w
# Simple diff with regular expressions
#
# usage: rxdiff <file_with_regexp> <file_to_check>
#
use strict;

sub main()
{
    if( @ARGV < 2) {
        die "usage: rxdiff <file_with_regex> <file_to_check>\n";
    }
    
    my $file1 = $ARGV[0];
    my $file2 = $ARGV[1];
    my $pattern;
    my $line;
    my $result = 0;
    
    open F1, "<$file1" or die "ERROR: unable to open $file1.\n";
    open F2, "<$file2" or die "ERROR: unable to open $file2.\n";

    while( $pattern = <F1> ) {
        my $line = <F2>;

        if( not $line =~ /$pattern/ ) {
            print "<< ".$pattern;
            print ">> ".$line;
            $result = 1;
        } else {
#            print "== ".$pattern;
#            print "== ".$line;
        }
    }
    
    close F2;
    close F1;

    exit $result;
}

main();
