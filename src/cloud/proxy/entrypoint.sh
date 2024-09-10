#!/bin/sh -e

# Copyright 2018- The Pixie Authors.
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
# SPDX-License-Identifier: Apache-2.0

if [ -n "$PL_DOMAIN_NAME" ]; then
    sed -i -e "s/[@]PL_DOMAIN_NAME[@]/$PL_DOMAIN_NAME/" /usr/local/openresty/nginx/conf/nginx.conf
else
   echo "PL_DOMAIN_NAME undefined, exiting"
   exit 1
fi

if [ -n "$PL_KUBE_DNS_SERVICE" ]; then
    sed -i -e "s/[@]PL_KUBE_DNS_SERVICE[@]/$PL_KUBE_DNS_SERVICE/" /usr/local/openresty/nginx/conf/nginx.conf
fi    

/usr/bin/openresty -g "daemon off;"
