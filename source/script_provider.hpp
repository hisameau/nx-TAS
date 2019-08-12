#pragma once
#include <string>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <istream>
#include "script_init.hpp"

class ScriptProvider
{
private:
    mutable std::mutex mutex;
    std::queue<std::shared_ptr<struct controlMsg>> internalQueue;

protected:
    void pushToQueue(std::shared_ptr<struct controlMsg> msg)
    {
        std::lock_guard<std::mutex> guard(mutex);
        return internalQueue.push(msg);
    }

    std::shared_ptr<struct controlMsg> pullFromQueue()
    {
        std::lock_guard<std::mutex> guard(mutex);
        std::shared_ptr<struct controlMsg> msg = internalQueue.front();
        internalQueue.pop();
        return msg;
    }

public:
    bool queueIsEmpty()
    {
        std::lock_guard<std::mutex> guard(mutex);
        return internalQueue.empty();
    }
    int queueSize()
    {
        std::lock_guard<std::mutex> guard(mutex);
        return internalQueue.size();
    }

    std::shared_ptr<struct controlMsg> nextLine()
    {
        return pullFromQueue();
    }
    bool hasNextLine()
    {
        return queueIsEmpty();
    }

    virtual void populateQueue() {};

};

class LineStreamScriptProvider: public ScriptProvider
{
private:
    std::istream& stream;

protected:
    virtual void afterEOF()
    {}
    virtual bool shouldPopulate()
    {
        return queueSize() < 15;
    }

public:
    LineStreamScriptProvider(std::istream& mStream)
    : stream(mStream)
    {}

    bool hasNextLine()
    {
        return !(stream.eof() && queueIsEmpty());
    }
    void populateQueue()
    {
        if(shouldPopulate())
        {
            int frame;
            std::string keyStr, lStickStr, rStickStr;
            while(queueSize() < 30 && !stream.eof())
            {
                stream >> frame >> keyStr >> lStickStr >> rStickStr;
                struct controlMsg msg = lineAsControlMsg(frame, keyStr, lStickStr, rStickStr);
                std::shared_ptr<struct controlMsg> sharedPtr = std::make_shared<struct controlMsg>(msg);
                pushToQueue(sharedPtr);
            }
            if(stream.eof())
            {
                afterEOF();
            }
        }
    }

};

class LineFileScriptProvider: public LineStreamScriptProvider
{
    std::ifstream stream;

protected:
    void afterEOF()
    {
        stream.close();
    }
    bool shouldPopulate()
    {
        return stream.is_open() && queueSize() < 15;
    }

public:
    LineFileScriptProvider(std::string fileName)
    : LineStreamScriptProvider(stream), stream(fileName)
    {}

};