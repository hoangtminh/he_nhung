#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void setNumPlayers(int count)
    {
        numPlayers = count;
    }

    int getNumPlayers() const
    {
        return numPlayers;
    }

protected:
    ModelListener* modelListener;
    int numPlayers;
};

#endif // MODEL_HPP
