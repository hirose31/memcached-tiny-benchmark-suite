#!/usr/bin/env perl

use strict;
use warnings;
use utf8;
use Carp;
use YAML;
use IO::File;
STDOUT->autoflush(1); STDERR->autoflush(1);
use Getopt::Long;
use Pod::Usage;
use Cache::Memcached;
use Readonly;
use Time::HiRes qw(gettimeofday);
use POSIX qw(strftime);

BEGIN {
    my $debug_flag = $ENV{SMART_COMMENTS} || $ENV{SMART_DEBUG} || $ENV{SC};
    if ($debug_flag) {
        my @p = map { '#'x$_ } ($debug_flag =~ /([345])\s*/g);
        use UNIVERSAL::require;
        Smart::Comments->use(@p);
    }
}

use Encode;
binmode STDERR, ':encoding(euc-jp)';
binmode STDOUT, ':encoding(euc-jp)';

sub p(@) { print YAML::Dump(\@_); }

sub Cache::Memcached::stats_misc {
    my($self) = @_;
    return $self->stats(['misc'])->{hosts}{ $self->{servers}[0] }{misc};
}


MAIN: {
    my %opt;
    my $interval = 3;
    my($server, $mc);
    Getopt::Long::Configure("bundling");
    GetOptions(\%opt,
               'server|s=s'   => \$server,
               'interval|i=i' => \$interval,
               'help|h|?'     => sub{ pod2usage(-verbose=>1) },
              ) or pod2usage();
    ### opt: %opt
    pod2usage() unless $server;
    $server .= ':11211' unless $server =~ /:/;
    ### server: $server
    ### interval: $interval

    $mc = Cache::Memcached->new(servers => [ $server ],
                                debug   => 0,
                               ) or croak $!;
    $mc->set_cb_connect_fail(
        sub{
            croak join(' ',@_).": failed to connect: $!";
        });

    my($stats, $prev_stats);
    $prev_stats = $mc->stats_misc();
#    p $prev_stats; exit;
    my $count = 0;
    while (1) {
        $stats = $mc->stats_misc();
#        printf "%d %d\n", $prev_stats->{uptime}, $stats->{uptime};
        $count %= 20;
        print_legend() if $count++ == 0;
        format_diff( diff_stats($prev_stats, $stats) );
        $prev_stats = $stats;
        sleep $interval;
    }

}

sub print_legend {
    print "                   read B/s  write B/s   set/s  get/s tot_item/s   stime  utime\n";
}

sub format_diff {
    my($diff) = @_;
    return if $diff->{uptime} == 0;

    my ($sec, $microsec) = gettimeofday;
    printf("%-8s.%06d: %10.2f %10.2f  %6d %6d  %9.2f  %6.3f %6.3f\n",
           strftime("%02H:%02M:%02S", localtime($sec)),
           $microsec,

           $diff->{bytes_read}/$diff->{uptime},
           $diff->{bytes_written}/$diff->{uptime},

           $diff->{cmd_set}/$diff->{uptime},
           $diff->{cmd_get}/$diff->{uptime},

#           $diff->{curr_items}/$diff->{uptime},
#           $diff->{bytes}/$diff->{uptime}, # bytes = curr_bytes
           $diff->{total_items}/$diff->{uptime},

           $diff->{rusage_system},
           $diff->{rusage_user},
           );
}

sub diff_stats {
    my($prev, $now) = @_;
    my $diff;
    my %skip = map { $_=>1 } qw(version replication);

    for my $key (grep { ! $skip{$_} } keys %{ $now }) {
        $diff->{ $key } = $now->{ $key } - $prev->{ $key };
    }

    return $diff;
}

__END__

=encoding utf-8

=head1 NAME

B<monitor-mcd.pl> - monitor memcached

=head1 SYNOPSIS

B<monitor-mcd.pl> B<-s> HOST[:PORT] B<-i> n

  $ monitor-mcd.pl -s server1

=head1 DESCRIPTION

monitor memcached blah blah blah

=head1 OPTIONS

=over 4

=item B<-s> B<--server> HOST[:PORT]

host name. defaut port is 11211.

=item B<-i> B<--interval> n

interval between monitor in seconds.

=back

=cut

# for Emacsen
# Local Variables:
# mode: cperl
# cperl-indent-level: 4
# indent-tabs-mode: nil
# coding: utf-8
# End:

# vi: set ts=4 sw=4 sts=0 :
