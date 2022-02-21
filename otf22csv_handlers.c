/*
    This file is part of Akypuera

    Akypuera is free software: you can redistribute it and/or modify
    it under the terms of the GNU Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Akypuera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Public License for more details.

    You should have received a copy of the GNU Public License
    along with Akypuera. If not, see <http://www.gnu.org/licenses/>.
*/
#include "otf22csv.h"
#include <inttypes.h>

static int *parameter_hash = NULL;
static int parameter_hash_current_size = 0;

static char **string_hash = NULL;
static int string_hash_current_size = 0;

static int *region_name_map = NULL;
static int region_name_map_current_size = 0;

static char **type_tree_hash = NULL;
static int type_tree_current_size = 0;

/* time_to_seconds */
static double time_to_seconds(double time, double resolution)
{
  static double first_time = -1;
  if (first_time == -1){
    first_time = time;
  }
  return (time - first_time) / resolution;
}

/* Definition callbacks */
OTF2_CallbackCode otf22csv_unknown (OTF2_LocationRef locationID, OTF2_TimeStamp time, void *userData, OTF2_AttributeList* attributes)
{
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_clock_properties (void *userData, uint64_t timerResolution, uint64_t globalOffset, uint64_t traceLength)
{
  otf2paje_t* data = (otf2paje_t*) userData;
  data->time_resolution = timerResolution;
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_string (void *userData, OTF2_StringRef self, const char *string)
{
  string_hash_current_size = self + 1;
  string_hash = (char**)realloc (string_hash, string_hash_current_size*sizeof(char*));
  string_hash[self] = (char*) malloc (strlen(string)+1);
  strcpy (string_hash[self], string);
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_region (void *userData, OTF2_RegionRef self, OTF2_StringRef name, OTF2_StringRef canonicalName, OTF2_StringRef description, OTF2_RegionRole regionRole, OTF2_Paradigm paradigm, OTF2_RegionFlag regionFlags, OTF2_StringRef sourceFile, uint32_t beginLineNumber, uint32_t endLineNumber)
{
  region_name_map_current_size = self + 1;
  region_name_map = (int*) realloc (region_name_map, region_name_map_current_size*sizeof(int));
  region_name_map[self] = name;
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_location_group_flat (void *userData, OTF2_LocationGroupRef self, OTF2_StringRef name, OTF2_LocationGroupType locationGroupType, OTF2_SystemTreeNodeRef systemTreeParen)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_location_group_hostfile (void *userData, OTF2_LocationGroupRef self, OTF2_StringRef name, OTF2_LocationGroupType locationGroupType, OTF2_SystemTreeNodeRef systemTreeParent)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_location_group (void *userData, OTF2_LocationGroupRef self, OTF2_StringRef name, OTF2_LocationGroupType locationGroupType, OTF2_SystemTreeNodeRef systemTreeParent)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_location (void* userData, OTF2_LocationRef self, OTF2_StringRef name, OTF2_LocationType locationType, uint64_t numberOfEvents, OTF2_LocationGroupRef locationGroup)
{
  otf2paje_t* data = (otf2paje_t*) userData;

  if ( data->locations->size == data->locations->capacity )
  {
      return OTF2_CALLBACK_INTERRUPT;
  }
  
  data->locations->members[ data->locations->size++ ] = self;
  
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_system_tree_node_hostfile (void *userData, OTF2_SystemTreeNodeRef self, OTF2_StringRef name, OTF2_StringRef className, OTF2_SystemTreeNodeRef parent)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_system_tree_node (void *userData, OTF2_SystemTreeNodeRef self, OTF2_StringRef name, OTF2_StringRef className, OTF2_SystemTreeNodeRef parent)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_system_tree_node_property (void *userData, unsigned int systemTreeNode, unsigned int ignore, unsigned char name, union OTF2_AttributeValue_union value)
{
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_global_def_system_tree_node_domain( void* userData, OTF2_SystemTreeNodeRef systemTreeNode, OTF2_SystemTreeDomain  systemTreeDomain )
{
  return OTF2_CALLBACK_SUCCESS;
}

/* Events callbacks */
OTF2_CallbackCode otf22csv_enter (OTF2_LocationRef locationID, OTF2_TimeStamp time, void *userData, OTF2_AttributeList* attributes, OTF2_RegionRef regionID)
{
  otf2paje_t* data = (otf2paje_t*) userData;
  int index;

  //search the correct index of the locationID
  for (index = 0; index < data->locations->size; index++){
    if (data->locations->members[index] == locationID) break;
  }

  /* copy from metrics to enter_metrics */
  //printf("%d %s ", index, __FUNCTION__);
  for (int i = 0; i < data->metrics_n[index]; i++){
    data->enter_metrics[index][i] = data->metrics[index][i];
    //printf("%d (%d) ",
//	   data->enter_metrics[index][i].id,
//	   data->enter_metrics[index][i].value);
  }
  // printf("\n");
  data->enter_metrics_n[index] = data->metrics_n[index];
  /* reset metrics */
  data->metrics_n[index] = 0;

  //Allocate memory if that is not yet the case
  if (data->last_enter_metric[index] == NULL){
    data->last_enter_metric[index] = malloc(MAX_IMBRICATION * sizeof(uint64_t**));
    for (uint8_t j = 0; j < MAX_IMBRICATION; j++){
      data->last_enter_metric[index][j] = malloc(data->number_of_metrics * sizeof(uint64_t));
    }
  }
  
  //Define last "enter" event metrics in the current imbrication level
  for ( uint8_t j = 0; j < data->number_of_metrics; j++ ){
    data->last_enter_metric[index][data->last_imbric[index]][j] = data->last_metric[index][j];
  }
  
  data->last_timestamp[index][data->last_imbric[index]] = time_to_seconds(time, data->time_resolution);
  data->last_imbric[index]++;
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_leave (OTF2_LocationRef locationID, OTF2_TimeStamp time, void *userData, OTF2_AttributeList* attributes, OTF2_RegionRef regionID)
{
  otf2paje_t* data = (otf2paje_t*) userData;
  const char *state_name = string_hash[region_name_map[regionID]];
  int index;
  //search the correct index of the locationID
  for (index = 0; index < data->locations->size; index++){
    if (data->locations->members[index] == locationID) break;
  }

  /* copy from metrics to leave_metrics */
  //printf("%d %s ", index, __FUNCTION__);
  for (int i = 0; i < data->metrics_n[index]; i++){
    data->leave_metrics[index][i] = data->metrics[index][i];
    //printf("%d (%d) ",
//	   data->leave_metrics[index][i].id,
//	   data->leave_metrics[index][i].value);
  }
  //printf("\n");
  data->leave_metrics_n[index] = data->metrics_n[index];
  /* reset metrics */
  data->metrics_n[index] = 0;

  /* compute the difference of the enter/leave metrics */
  if (data->enter_metrics_n[index] != data->leave_metrics_n[index]){
    printf("Error, number of metrics in enter and leave are different.\n");
    exit(1);
  }
  metric_t diff_metric[MAX_METRICS];
  int n = data->enter_metrics_n[index];
  //printf("%d %s DIFF ", index, __FUNCTION__);
  for (int i = 0; i < n; i++){
    if (data->leave_metrics[index][i].id != data->enter_metrics[index][i].id){
      printf("Error, metrics are unaligned in enter and leave.\n");
      exit(1);
    }
    diff_metric[i].id = data->leave_metrics[index][i].id;
    diff_metric[i].value = data->leave_metrics[index][i].value -
      data->enter_metrics[index][i].value;
    //printf("%d (%d) ", diff_metric[i].id, diff_metric[i].value);
  }
  //printf("\n");

  //Reduce imbrication since we are back one level
  //This has to be done prior to everything
  data->last_imbric[index]--;
  double before = data->last_timestamp[index][data->last_imbric[index]];
  double now = time_to_seconds(time, data->time_resolution);

  //Get the last_metric values
  uint64_t *my_last_metrics = data->last_metric[index];
  //Get the last "enter" metric values
  uint64_t *my_last_enter_metrics = data->last_enter_metric[index][data->last_imbric[index]];
  //Calculate the difference of these metrics
  uint64_t *diff = malloc(sizeof(uint64_t) * data->number_of_metrics);
  for ( uint8_t j = 0; j < data->number_of_metrics; j++ ){
    diff[j] = my_last_metrics[j] - my_last_enter_metrics[j];
  }
  
  if (!arguments.dummy){
    int i;
    char *safe_state_name = NULL;
    //verify if there are commas
    for (i = 0; i < strlen(state_name); i++) {
      if(state_name[i] == ',') break;
    }
    if (i == strlen(state_name)) {
      //there are NO commas
      safe_state_name = (char*)malloc((strlen(state_name)+1) * sizeof(char));
      bzero(safe_state_name, strlen(state_name)+1);
      strncpy(safe_state_name, state_name, strlen(state_name));
    }else{
      //there are commas, put double quotes
      safe_state_name = (char*)malloc((strlen(state_name)+3) * sizeof(char));
      bzero(safe_state_name, strlen(state_name)+2);
      safe_state_name[0] = '\"';
      strncpy(safe_state_name+1, state_name, strlen(state_name));
      int len = strlen(safe_state_name);
      safe_state_name[len] = '\"';
      safe_state_name[len+1] = '\0';
    }
    printf("%ld,%f,%f,%d,%s", locationID, before, now, data->last_imbric[index], safe_state_name);
    free(safe_state_name);

    if (data->parameters_n[index] != 0){
      printf(",");
      for(uint8_t j = 0; j < data->parameters_n[index]; j++ ){
	int aux = data->parameters[index][j];
	printf("%s", string_hash[aux]);
	if(j+1 < n){
	  printf(",");
	}
      }
      data->parameters_n[index] = 0;
    }
    if (n == 0){
      printf("\n");
    }else{
      printf(",");
      //Print the difference of the metrics
      for ( uint8_t j = 0; j < n; j++ ){
	printf("%"PRIu64, diff_metric[j].value);
	if (j+1 < n){
	  printf(",");
	}
      }
      printf("\n");
    }
  }
  free(diff);
  data->last_timestamp[index][data->last_imbric[index]] = now;
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_print_metric( OTF2_LocationRef locationID, OTF2_TimeStamp time, void* userData, OTF2_AttributeList* attributes, OTF2_MetricRef metric, uint8_t numberOfMetrics, const OTF2_Type* typeIDs, const OTF2_MetricValue* metricValues )
{
  otf2paje_t* data = (otf2paje_t*) userData;
  int index;

  //search the correct index of the locationID
  for (index = 0; index < data->locations->size; index++){
    if (data->locations->members[index] == locationID) break;
  }

  //register metric for later utilization
  int n = data->metrics_n[index];
  data->metrics[index][n].id = metric;
  data->metrics[index][n].value = metricValues[0].unsigned_int;
  data->metrics_n[index]++;

  //printf("%d %s %d %d %d (%d)\n", index, __FUNCTION__, metric, n, data->metrics[index][n].id, data->metrics[index][n].value);

  data->number_of_metrics = numberOfMetrics;
  if (data->last_metric[index] == NULL){
    data->last_metric[index] = malloc(data->number_of_metrics * sizeof(uint64_t));
  }
  uint64_t *my_metrics = data->last_metric[index];
   
  for ( uint8_t j = 0; j < numberOfMetrics; j++ ){
    my_metrics[j] = metricValues[j].unsigned_int;
  }
  return OTF2_CALLBACK_SUCCESS;
}


OTF2_CallbackCode otf22csv_global_def_parameter ( void*              userData,
						  OTF2_ParameterRef  self,
						  OTF2_StringRef     name,
						  OTF2_ParameterType parameterType )
{
  parameter_hash_current_size = self + 1;
  parameter_hash = (int*)realloc (parameter_hash, parameter_hash_current_size*sizeof(int));
  parameter_hash[self] = name;
  //printf("%s %d -> %d -> %s\n", __func__, self, name, string_hash[name]);
  return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode otf22csv_parameter_string ( OTF2_LocationRef    locationID,
					      OTF2_TimeStamp      time,
					      void*               userData,
					      OTF2_AttributeList* attributeList,
					      OTF2_ParameterRef   parameter,
					      OTF2_StringRef      string )
{
  otf2paje_t* data = (otf2paje_t*) userData;
  int index;

  //search the correct index of the locationID
  for (index = 0; index < data->locations->size; index++){
    if (data->locations->members[index] == locationID) break;
  }

  int position = data->parameters_n[index];
  if(position > MAX_PARAMETERS) {
    fprintf(stderr, "Increase MAX_PARAMETERS to at least %d. Recompile.\n", position);
    exit(1);
  }
  data->parameters[index][position] = string;
  data->parameters_n[index]++;
  //printf("PARAMETER STRING %s %ld %s\n", __func__, locationID, string_hash[string]);
  return OTF2_CALLBACK_SUCCESS;
}
