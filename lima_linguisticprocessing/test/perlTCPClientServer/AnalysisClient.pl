#   Copyright 2002-2013 CEA LIST
#    
#   This file is part of LIMA.
#
#   LIMA is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   LIMA is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with LIMA.  If not, see <http://www.gnu.org/licenses/>
#!/usr/bin/perl -w
# MemoryCollocationClient.pl
# client d'essai de gestionnaire de collocations
# Ga\"el de Chalendar
# 26/10/2000

# CHANGELOG
# 26/10/2000 :
#  - creation d'apres un exemple de man:perlipc(3)
#  - toutes les variables "en dur"

# TODO
#  - mettre les variables et paths en parametres

# Emplacement des modules de gestion du reseau
use strict;
use IO::Socket;
my ($host, $port, $kidpid, $handle, $line);
my $EOL="\n";

unless (@ARGV == 2) { die "usage: $0 host port" }
($host, $port) = @ARGV;

# create a tcp connection to the specified host and port
$handle = IO::Socket::INET->new(Proto     => "tcp",
                                PeerAddr  => $host,
                                PeerPort  => $port)
       or die "can't connect to port $port on $host: $!";

$handle->autoflush(1);              # so output gets there right away
print STDERR "[Connected to $host:$port]\n";

# split the program into two processes, identical twins
die "can't fork: $!" unless defined($kidpid = fork());

# the if{} block runs only in the parent process
if ($kidpid)
{
  # copy the socket to standard output
  while (defined ($line = <$handle>))
  {
    print STDOUT $line;
    if ($line =~ /BYE|KILLED/) {last;}
  }
  kill("TERM", $kidpid);                  # send SIGTERM to child
}
# the else{} block runs only in the child process
else
{
  # copy standard input to the socket
  while (defined ($line = <STDIN>))
  {
    chomp $line;
    print $handle "$line$EOL";
  }
}