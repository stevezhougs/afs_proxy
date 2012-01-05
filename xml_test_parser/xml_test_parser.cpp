//
// httpget.cpp
//
// $Id: //poco/1.4/Net/samples/httpget/src/httpget.cpp#1 $
//
// This sample demonstrates the HTTPClientSession class.
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include "Poco/ThreadPool.h"
#include "Poco/StreamCopier.h"
#include "Poco/Random.h"
#include "Poco/Path.h"
#include "Poco/File.h"
#include <iostream>
#include <sstream>
#include "PointerCollection.h"
#include "Filehash.h"
#include "Chunk.h"
#include "HTTPUtils.h"
#include <fstream>



using Poco::Random;
using Poco::StreamCopier;
using Poco::Path;
using Poco::File;

int main(int argc, char** argv)
{
    Filehash* f;
    std::string pointer;
    std::vector<std::string> validEndpoints;

    pointer = HTTPUtils::get("http://voldemort.cs.cf.ac.uk:7048/dl/meta/pointer/");

    if(pointer == "-1"){
        return 1;
    }

    PointerCollection* p = new PointerCollection(pointer.c_str());

    list<DataPointer>::iterator i;

    for(i = p->dp->begin(); i != p->dp->end(); i++){
        list<Endpoint>::iterator e;

        for(e = (*i).ep->begin(); e != (*i).ep->end(); e++){
            std::stringstream ss;
            ss << (*e).meta << "?filehash=" << (*i).dd->id;
            std::string out = HTTPUtils::get(ss.str());

            if(out != "-1"){
                f = new Filehash(out.c_str());
                std::cout << f->toString();
                (*e).accessible = true;
                validEndpoints.push_back((*e).url);
            }
            std::cout << "\n";
        }
    }

    if(!f){
        return -1;
    }

    int noValidEndpoints = (int) validEndpoints.size();
    
    Random r;

    //std::cout << r.next(noValidEndpoints);

    list<Segment>::iterator it;
    list<Chunk> chunks;
    
    for(it = f->seg->begin(); it != f->seg->end(); it++)
    {
        chunks.push_back(Chunk(validEndpoints,(*it).hash,(*it).start,(*it).end));
    }

    Poco::ThreadPool tp(2,8);

    list<Chunk>::iterator cit;

    for(cit = chunks.begin(); cit != chunks.end(); cit++)
    {
        while(tp.available() <= 0)
        {
            tp.collect();
        }

        tp.start((*cit));
    }

    Path path("C:\\Users\\keyz\\test.txt", Path::PATH_WINDOWS);
    File file(path);
    file.createFile();

    std::ofstream ostr(file.path().c_str(), std::ios::binary);

    for(cit = chunks.begin(); cit != chunks.end(); cit++)
    {
        std::ifstream istr((*cit).tmp.path().c_str(),std::ios::binary);
        StreamCopier::copyStream(istr,ostr);
    }


    ///TODO: check the chunk has downloaded. Check they're all there.

    tp.joinAll();

    std::cout << p->toString();
    return 0;
}

