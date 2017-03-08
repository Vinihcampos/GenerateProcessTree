#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <utility>
#include <fstream>
#include <boost/filesystem.hpp>
#include "json/json.h"

using namespace std;
using namespace boost::filesystem;

map<string, pair < string, set< string > > > processes;

bool is_number(const std::string & s){
    return !s.empty() && std::find_if(s.begin(),s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

Json::Value populate_json(string pid){
	Json::Value name;
	name["name"] = Json::Value(pid + " " + processes[pid].first);

	Json::Value children;
	if(!processes[pid].second.empty()){
		for(auto it = processes[pid].second.begin(); it != processes[pid].second.end(); ++it){
			children.append( populate_json( *it ) );
		}
	}
	Json::Value structure;
	structure["text"] = name;
	
	if(!children.isNull()) structure["children"] = children; 

	return structure; 
}

int main(int argn, char ** argc){
	
	int pid = 1;

	if(argn > 1){
		pid = atoi(argc[1]);
	}

	path p("/proc");

    directory_iterator end_itr;

    queue<string> directory_has_subprocesses;

    for (directory_iterator itr(p); itr != end_itr; ++itr) {

    	if( is_directory( itr->path() ) && is_number(itr->path().leaf().string()) ){
    		ifstream file(itr->path().string() + "/stat", ifstream::in);
    		std::vector<string> parsed(std::istream_iterator<string>(file), {});

    		string PID = "", NAME = "", PPID = "";
    		int counter = 1;
    		for(int i = 0; i < parsed.size(); ++i){
    			if((counter == 1 || counter == 3) && is_number(parsed[i])){
    				counter == 1 ? PID = parsed[i] : PPID = parsed[i];
    				counter++;
    			}else if(counter == 2 && !is_number(parsed[i])){
    				while(parsed[i][parsed[i].size()-1] != ')'){
    					NAME += parsed[i++] + " "; 
    				}
    				NAME += parsed[i];
    				counter++;
    			}
    			if(counter > 3) break;
    		} 

    		if( processes.find(  PID )  == processes.end()){
    			processes.insert( make_pair( PID , make_pair( NAME, set<string>() ) ) );
    		}else{
    			processes[  PID ].first = NAME;
    		}

    		if(PPID.compare("0")){
    			if(processes.find(  PPID )  == processes.end()){
	    			processes.insert( make_pair( PPID , make_pair( "", set<string>() ) ) );
	    		}
	    		processes[ PPID ].second.insert( PID );
    		}

            if(exists(itr->path().string() + "/task")){
                directory_has_subprocesses.push(itr->path().string() + "/task");
            }    		
    	}
    }

    while(!directory_has_subprocesses.empty()){
        string front = directory_has_subprocesses.front();
        directory_has_subprocesses.pop();

        for(directory_iterator itr(front); itr != end_itr; ++itr){

        	//cout << itr->path().string() << endl;

        	if( is_directory( itr->path() ) && is_number(itr->path().leaf().string()) ){
        		//cout << "Number path: " << itr->path().string() << endl;

	    		ifstream file(itr->path().string() + "/stat", ifstream::in);
	    		std::vector<string> parsed(std::istream_iterator<string>(file), {});

	    		string PID = "", NAME = "", PPID = "";
	    		int counter = 1;
	    		for(int i = 0; i < parsed.size(); ++i){
	    			if((counter == 1 || counter == 3) && is_number(parsed[i])){
	    				counter == 1 ? PID = parsed[i] : PPID = parsed[i];
	    				counter++;
	    			}else if(counter == 2 && !is_number(parsed[i])){
	    				while(parsed[i][parsed[i].size()-1] != ')'){
	    					NAME += parsed[i++] + " "; 
	    				}
	    				NAME += parsed[i];
	    				counter++;
	    			}
	    			if(counter > 3) break;
	    		} 

	    		//cout << "PID ("<<PID<<") | NAME ("<<NAME<<") | PPID ("<<PPID<<")\n";

	    		if( processes.find( PID )  == processes.end()){
	    			processes.insert( make_pair( PID , make_pair( NAME, set<string>() ) ) );
	    		}else{
	    			processes[ PID ].first = NAME;
	    		}

	    		if(PPID.compare("0")){
	    			if(processes.find(  PPID )  == processes.end()){
		    			processes.insert( make_pair( PPID , make_pair( "", set<string>() ) ) );
		    		}
		    		processes[ PPID ].second.insert( PID );
	    		}

	            if(exists(itr->path().string() + "/task")){            
	                directory_has_subprocesses.push(itr->path().string() + "/task");
	            }   		
	    	}
        }          
                
    }

    Json::Value tree;
    Json::Value chartValue;
    Json::Value connectors;
    Json::Value style;

    connectors["type"] = Json::Value("step");
    //style["stroke-width"] = Json::Value("2");
    //style["stroke"] = Json::Value("#ccc");
    //connectors["style"] = style;

    chartValue["container"] = Json::Value("#tree-simple");
    chartValue["rootOrientation"] = Json::Value("WEST");
    chartValue["nodeAlign"] = Json::Value("BOTTOM");
    chartValue["connectors"] = connectors;

    tree["chart"] = chartValue;
    
    tree["nodeStructure"] = populate_json(to_string(pid));

    //-------- Writing JSON ------------//
    Json::StyledWriter writer;

    std::ofstream ofl("page/json/tree.json");
    ofl << "var simple_chart_config = ";
    ofl << writer.write(tree);
    ofl << ";";
    ofl.close();

	return 0;
}