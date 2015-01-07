/// Id: //minode.cpp,v 1.10 2014-06-12 18:16:45-07 - - $

#include <cassert>
#include <iostream>

using namespace std;

#include "debug.h"
#include "inode.h"

int inode::next_inode_nr {1};


//**************INODE_FN's******************
inode::inode(inode_t init_type):
   inode_nr (next_inode_nr++), type (init_type)
{
   switch (type) {
      case PLAIN_INODE:
           contents = make_shared<plain_file>();
           break;
      case DIR_INODE:
           contents = make_shared<directory>();
           break;
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

int inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}

const map<string,inode_ptr>& inode::get_contents(){
  return  directory_ptr_of(contents)->get_dirents();
}
void inode::writefile(const wordvec& data){
  plain_file_ptr_of(contents)->writefile(data);
}


size_t inode::size(){
  if(type == PLAIN_INODE){
    return plain_file_ptr_of(contents)->size();
  }
    return directory_ptr_of(contents)->size();
}

inode_t inode::get_type(){
  return type;
}


//**************FILE_FN's******************
plain_file_ptr plain_file_ptr_of (file_base_ptr ptr) {
   plain_file_ptr pfptr = dynamic_pointer_cast<plain_file> (ptr);
   return pfptr;
}

directory_ptr directory_ptr_of (file_base_ptr ptr) {
   directory_ptr dirptr = dynamic_pointer_cast<directory> (ptr);
   return dirptr;
}


size_t plain_file::size() const {
   size_t size {};
   for(auto word : data){
     size += word.size();
   }
   if(data.size()!=0){
 
   size = size + data.size()-1 ;

   }
   DEBUGF ('i', "size = " << size);
   return size;
}


void plain_file::writefile (const wordvec& newdata) {
   DEBUGF ('i', newdata);
   data = newdata; 
}

string plain_file::readfile(){
     string str = ""; 
     for(auto elem : data){
          str = str + elem+ " ";
      }
      return str;
}

//**************DIRECTORY_FN's******************
size_t directory::size() const {
   size_t size {dirents.size()};
   DEBUGF ('i', "size = " << size);
   return size;
}

void directory::remove (const string& filename) {
     auto elem = dirents.find(filename);
     if(elem != dirents.end()){
       (*elem).second == nullptr;
       dirents.erase(elem);
     }else{
       throw yshell_exn("no such file / directory");
     }
     
   DEBUGF ('i', filename);
}

directory::directory(){
  dirents.insert({".",nullptr}); dirents.insert({"..",nullptr});
}

inode& directory::mkfile(const string& filename){
      inode_ptr i = make_shared<inode>(PLAIN_INODE);
      if(dirents.find(filename)== dirents.end()){
          dirents.insert({filename,i}); 
      }
      return *i; 
}

inode& directory::mkdir(const string& dirname){
      inode_ptr i = make_shared<inode>(DIR_INODE);
      if(dirents.find(dirname)==dirents.end()){
        dirents.insert({dirname,i});
      }

      return *i;
}

const map<string,inode_ptr>& directory::get_dirents(){
  return dirents;
}
void directory::set_dot(inode_ptr ip){
  dirents.at(".") = ip;

}
void directory::set_parent(inode_ptr ip){
  dirents.at("..") = ip;
}

//**************INODE_STATE_FN's******************

inode_state::inode_state() {
   DEBUGF ('i', "root = " << root << ", cwd = " << cwd
          << ", prompt = \"" << prompt << "\"");
   root = make_shared<inode>(DIR_INODE);
   cwd = root;
   directory_ptr_of(root->contents)->set_dot(root);
   directory_ptr_of(root->contents)->set_parent(root);
}

const inode_ptr inode_state::get_root(){
  return root;
}

const inode_ptr inode_state::get_cwd(){
  return cwd;
}

const map<string,inode_ptr>& inode_state::get_contents(){
  return  directory_ptr_of(cwd->contents)->get_dirents();
}

void inode_state::remove(const wordvec pathname,bool r,bool d){
  //start at root go down path
  auto i_ptr = get_cwd();
  if(r){auto i_ptr = get_root();} 
  if(pathname.size()==0) {root = nullptr;return;} 
  auto it = pathname.begin();
  for(;it!=pathname.end()-1;it++){
    auto p = i_ptr->get_contents().find(*it);
    //If no directory in path print and exit
    if( p == i_ptr->get_contents().end() ) {
        throw yshell_exn("no such path");
    }
    //If then switch current directory
    i_ptr = p->second;
  }
  //Destroy last file in path if it exists
   if(i_ptr->get_contents().find(*(pathname.end()-1)) != i_ptr->get_contents().end()){
         if(d && i_ptr->get_contents().find(*(pathname.end()-1))->second->get_type()==DIR_INODE){
                   throw yshell_exn("cannot rm directory");
         }else{
            try{
                directory_ptr_of(i_ptr->contents)->remove(*(pathname.end()-1)); 
            }catch(yshell_exn){
                 throw yshell_exn("no such file");
            }
         }
    }else{
       throw yshell_exn("no such file");
    }
}

void inode_state::set_cwd(const wordvec pathname, const bool r){
  //start at root go down path
  auto i_ptr = cwd; 
  if(r){i_ptr = root;} 
  auto it = pathname.begin();
  for(;it!=pathname.end();it++){
    auto p = i_ptr->get_contents().find(*it);
    //If no directory in path print and exit
    if( p == i_ptr->get_contents().end() ) {
      throw yshell_exn("no such path");
    }
    //If then switch current directory
    i_ptr = p->second;
  }
  cwd = i_ptr;
}

void inode_state::add_directory(const wordvec pathname,bool r){
  //start at root go down path
  auto i_ptr = get_cwd(); 
  if(r){auto i_ptr = get_root();} 
  auto it = pathname.begin();
  for(;it!=pathname.end()-1;it++){
    auto p = i_ptr->get_contents().find(*it);
    //If no directory in path print and exit
    if( p == i_ptr->get_contents().end() || p->second->get_type()==PLAIN_INODE ) {
            throw yshell_exn("no such path");
    }
    //If then switch current directory
    i_ptr = p->second;
  }
  //Create directory in the last path dir
   auto p = i_ptr->get_contents().find(*it);
   if(p != i_ptr->get_contents().end() ){
     throw yshell_exn("directory already exists");
  }else{
      auto new_dir = directory_ptr_of(i_ptr->contents)->mkdir(*it);
      directory_ptr_of(new_dir.contents)->set_dot(make_shared<inode>(new_dir)); 
      directory_ptr_of(new_dir.contents)->set_parent(i_ptr); 
  }
}

void inode_state::add_file(const wordvec& data, const wordvec& pathname,bool r){
  //start at root go down path
  auto i_ptr = get_cwd(); 
  if(r){auto i_ptr = get_root();} 
  auto it = pathname.begin();
  for(;it!=pathname.end()-1;it++){
    auto p = i_ptr->get_contents().find(*it);
    //If no directory in path print and exit
    if( p == i_ptr->get_contents().end() ) {
        throw yshell_exn("no such path");
    }
    //If then switch current directory
    i_ptr = p->second;
  }
  //Create file in the last part of the path
  if(i_ptr->get_contents().find(*it)!= i_ptr->get_contents().end() ){
     throw yshell_exn("file already exists");
  }else{
  directory_ptr_of(i_ptr->contents)->mkfile(*it).writefile(data);
  }
}

string inode_state::readfile(const wordvec pathname,bool r){
  auto i_ptr = get_cwd(); 
  if(r){auto i_ptr = get_root();} 
  auto it = pathname.begin();
  for(;it!=pathname.end()-1;it++){
    auto p = i_ptr->get_contents().find(*it);
    //If no directory in path print and exit
    if( p == i_ptr->get_contents().end() ) {
        throw yshell_exn("no such path");
    }
    //If then switch current directory
    i_ptr = p->second;
  }
  auto file = i_ptr->get_contents().find(*(pathname.end()-1));
  if(file != i_ptr->get_contents().end()){
      if(file->second->get_type() == DIR_INODE){
        throw yshell_exn("not a file");
        return "";
      }
        return plain_file_ptr_of(file->second->contents)->readfile();
  }else{
     throw yshell_exn("no such file");
     return "";
  }
}

void inode_state::set_prompt(const string& p){
    prompt = p;
}

const string& inode_state::get_prompt(){
    return prompt;
}
ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

