#!/bin/bash
#
# File:     slurm_submit.sh
# Author:   David Rebatto (david.rebatto@mi.infn.it)
#
# Revision history:
#    14-Mar-2012: Original release
#
#
# Copyright (c) Members of the EGEE Collaboration. 2004.
# See http://www.eu-egee.org/partners/ for details on the copyright
# holders.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

. `dirname $0`/blah_common_submit_functions.sh

bls_parse_submit_options "$@"
bls_setup_all_files

# Default values for configuration variables
slurm_std_storage=${slurm_std_storage:-/dev/null}
slurm_opt_prefix=${slurm_opt_prefix:-SBATCH}

# Write wrapper preamble
cat >$bls_tmp_file << end_of_preamble
#!/bin/bash
# SLURM job wrapper generated by `basename $0`
# on `/bin/date`
#
# stgcmd = $bls_opt_stgcmd
# proxy_string = $bls_opt_proxy_string
# proxy_local_file = $bls_proxy_local_file
#
# SLURM directives:
#$slurm_opt_prefix -o $slurm_std_storage
#$slurm_opt_prefix -e $slurm_std_storage
end_of_preamble

# Add site specific directives
bls_local_submit_attributes_file=${blah_libexec_directory}/slurm_local_submit_attributes.sh
bls_set_up_local_and_extra_args

# Write SLURM directives according to command line options
# handle queue overriding
[ -z "$bls_opt_queue" ] || grep -q "^#$slurm_opt_prefix -p" $bls_tmp_file ||
  echo "#$slurm_opt_prefix -p $bls_opt_queue" >> $bls_tmp_file

# Input sandbox setup
bls_fl_subst_and_dump  inputsand "scp `hostname -f`:@@F_LOCAL @@F_REMOTE" >> $bls_tmp_file

# The wrapper's body...
bls_add_job_wrapper

# Output sandbox setup
echo "# Copy the output file back..." >> $bls_tmp_file
bls_fl_subst_and_dump outputsand "scp @@F_REMOTE `hostname -f`:@@F_LOCAL" >> $bls_tmp_file

if [ "x$bls_opt_debug" = "xyes" ]; then
  echo "Submit file written to $bls_tmp_file"
  exit 
fi

###############################################################
# Submit the script
###############################################################

datenow=`date +%Y%m%d`
jobID=`${slurm_binpath}/sbatch $bls_tmp_file | sed 's/Submitted batch job //'`
retcode=$?
if [ "$retcode" != "0" ] ; then
  rm -f $bls_tmp_file
  exit 1
fi

# Compose the blahp jobID ("slurm" + metadata + slurm jobid)
blahp_jobID="slurm/${datenow}/${jobID}"

if [ "x$job_registry" != "x" ]; then
  now=$((`date +%s` - 1))
  ${blah_sbin_directory}/blah_job_registry_add "$blahp_jobID" "$jobID" 1 $now "$bls_opt_creamjobid" "$bls_proxy_local_file" "$bls_opt_proxyrenew_numeric" "$bls_opt_proxy_subject"
fi

echo "BLAHP_JOBID_PREFIX$blahp_jobID"

bls_wrap_up_submit

exit $retcode

