#!/usr/bin/perl -s
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
use utf8
binmode STDOUT, ":utf8";

open (SOURCE,"<:utf8","$ARGV[0]") || die ("Impossible d'ouvrir $ARGV[0]");
open (OUT,">>:utf8","$ARGV[1]") || die ("Impossible d'ouvrir $ARGV[1]");

if ($main::h || $main::help || scalar(@ARGV)!=2) {
  print "USAGE : xmlforms [OPTIONS] inputfile outputfile\n";
  print "where [OPTIONS] are :\n";
  print "  -h or -help : print usage\n";
  print "  -desacc=[yes|no] : specify desacc attribute for entries. default is none, that equals 'yes'\n";
  print "  -entryop=[add|replace|delete] : specify op attribute for entries. default is none, that equals 'add'\n";
  print "  -lingop=[add|replace|delete] : specify op attribute for linginfos. default is none, that equals 'add'\n";
}

my $form = "";
my $lemma = "";
my $norm = "";

my $count = 0;
my $icount = 0;

# Does not bufferize outputs on stdout, for seeing progress
my $oldfh = select(STDOUT);
$| = 1;
select($oldfh);


while (<SOURCE>)
{
  chop();
  s/&/&amp;/g;
  s/"/&quot;/g;
  s/</&lt;/g;
  s/>/&gt;/g;
  if ($_ eq "") { next;}
  @data=split(/	/);
  if (/#/)  { next;}	# pour autoriser les commentaires 
  if (/^\s+$/)  { next;}	# pour autoriser les lignes vides 

 if (scalar(@data) !=4) {
    print "xmlform: Invalid line $_\n";
    next;
  }


  if ($data[0] ne $form) {
    $form=$data[0];
    if ($count > 0) {
      print OUT "  </i>\n";
      print OUT "</entry>\n";
    }
    print OUT "<entry k=\"$form\"";
    if ($main::desacc) {
      print OUT " desacc=\"$main::desacc\"";
    }
    if ($main::entryop) {
      print OUT " op=\"$main::entryop\"";
    }
    print OUT ">\n";
    $icount = 0;
    $count++;
    if (($count % 10000) == 0) {
      print "\n$count";
    }
  }
  if (($icount==0) || ($data[1] ne $lemma) || ($data[2] ne $norm)) {
    $lemma=$data[1];
    $norm=$data[2];
    if ($icount > 0) {
      print OUT "  </i>\n";
    }
    print OUT "  <i";
    if ($lemma ne "") {
      print OUT " l=\"$lemma\"";
    }
    if ($norm ne "") {
      print OUT " n=\"$norm\"";
    }
    if ($main::lingop) {
      print OUT " op=\"$main::lingop\"";
    }
    print OUT ">\n";
    $icount++;
  }
  print OUT "    <p v=\"$data[3]\"/>\n";
}
if ($count > 0) {
  print OUT "  </i>\n";
  print OUT "</entry>\n";
}
print "\n";
