#include "Matrix.h"
#include "game.h"
#include "audio.h"
#include "matchingSquareManager.h"
#include "eraseManager.h"
Matrix::Matrix(Game* g)
{
	
	parent=g;
	resetSourceDestination();
	initiateTags();
	initiateSquares();
	if(rand()%2==0)
		initiateMovementDuration=(float)6*rand()/RAND_MAX;
	else
		initiateMovementDuration=initiateAnimationDuration;
	eraseManager::reset();
	matchingSquareManager::clear();
	refreshTable();
}

Matrix::~Matrix(void)
{
	delete rTA;
}
void Matrix::resetTable()
{
	if(table==NULL)
		return;
	
	for (int i=NumberOfRows-1; i>=0; i--)
		for(int j=0; j<NumberOfColumns; j++)
			if(table[i][j].getMetaData()!=NULL)
			{
				parent->RemoveChild(table[i][j].getMetaData()->getData());
				delete table[i][j].getMetaData()->getData();
			}
}
void Matrix::exchange(float x, float y)
{
	
	point p; p.X=x; p.Y=y;
	coordination c[9];
	if(!animateExchange(p))
		return;
	if(!checkNeighbours())
	{
		resetSourceDestination();
		return;
	}

	exchangeTagsBetweenSourceAndDes();
	//how many cells must be erased
	int numberOfMatchCellsInSource=0,numberOfMatchCellsInDestination=0;
	numberOfMatchCellsInSource=returnMatchCells(source,c);
	
	if(numberOfMatchCellsInSource>0)
		matchingSquareManager::add(c,numberOfMatchCellsInSource);
	numberOfMatchCellsInDestination=returnMatchCells(destination,c);
	if(numberOfMatchCellsInDestination>0)
		matchingSquareManager::add(c,numberOfMatchCellsInDestination);
	//exchanging state
	move();
	parent->SetInputActive(false);
	localTimer->setTimerState(localTimer->exchanging);
	localTimer->setLength(exchangeDuration);
	localTimer->Resume();	
}
void Matrix::initiateTags()
{
	fillRandomly();
	smoothTable();
}
void Matrix::initiateSquares()
{
	specifyAnchorOfTableCells();
	makeSquares();
}
void Matrix::specifyAnchorOfTableCells()
{
	float x,y;
	for (int i=NumberOfRows-1; i>=0; i--)
		for(int j=0; j<NumberOfColumns; j++)
		{
			y=Area.getarea_Anchor().Y-(NumberOfRows-i-1)*Area.getSquareLength()-Area.getSquareLength()/2;
			x=Area.getarea_Anchor().X+ j*Area.getSquareLength()+Area.getSquareLength()/2;
			table[i][j].setAnchor(x,y);
		}
}
void Matrix::makeSquares()
{
	square* s;
	point p;
	for (int i=NumberOfRows-1; i>=0; i--)
		for(int j=0; j<NumberOfColumns; j++)
		{
			p.Y=Area.getarea_Anchor().Y- NumberOfRows*Area.getSquareLength();
			p.X=table[i][j].getAnchor().X;
			s=new square(table[i][j].mapValuesToAnimals(),p, table[i][j].getAnchor(), Area.getSquareLength(),parent->GetTweener());
			table[i][j].setMetaData(s);
			parent->AddChild(s->getData());
		}
}
void Matrix::initiateShow()
{
	localTimer=new Timer(0.0f, 0, &Matrix::timerEvent, (void*)this);
	
	parent->GetTimers()->Add(localTimer);
	rTA=new realizeTableAnimation();
		
	 
}
 void Matrix::animateFirstCoordination(int currentColumn)
 {
	 int currentRow=rTA->getRow();

	 table[currentRow][currentColumn].getMetaData()->getData()->m_IsVisible=true;
	 table[currentRow][currentColumn].getMetaData()->move(initiateMovementDuration);
 }
void Matrix::timerEvent(Timer* timer, void* userData)
{
	Matrix* m=(Matrix*) userData;

	switch (timer->getTimerState())
	{
	case 0:    //it means timer->initiateAnimate
		m->animateInitialization(timer);
		break;
		case 1:  //it means timer->transition
			timer->setTimerState(timer->readyToExchange);
			timer->Pause();
			m->parent->SetInputActive(true);
		break;
		case 2:  //it means timer->exchanging
			if(matchingSquareManager::isEmpty()) 
			{		
				timer->setTimerState(timer->RollBacking);
				timer->setLength(0.0f);
			}
			else
			{
				timer->setLength(0.0f);
				timer->setTimerState(timer->erasing);
			}
		break;

		case 4:   //it means timer->erasing
			m->eraseAfterDelay();
		break;
		case 5 :   //it means timer->RollBacking
			m->rollback();
		break;

		

		case 6:  //it means timer->ReadyToFill
			m->ReadyToFill();
			timer->setTimerState(timer->fillEmptySquare);
			//this part doesnt need to use break in order to implement immediate transition
			timer->setLength(MoveDownDuration,0);
		case 7:    //it means timer->fillEmptySquare
			m->fillEmptySquare();
		break;

	}
}
void Matrix::fillRandomly()
{
	srand((unsigned)time(NULL));
	for(int i=0; i<NumberOfRows; i++)
		for(int j=0; j<NumberOfColumns; j++)
		table[i][j].setTag((rand()%MaxDomain)+1);
}
void Matrix::smoothTable()
{
	coordination c;
	srand((unsigned)time(NULL));
	int numberOfMatchedElements=0;
	coordination matchingsquare[9];
	//increase the chance of possible moves in vertical direction
	for(int j=0; j<NumberOfColumns; j++)
	{
		for(int i=0; i<NumberOfRows-2; i++)
		{	
			if(table[i][j].getTag()!=table[i+1][j].getTag() && table[i+1][j].getTag()!=table[i+2][j].getTag() && table[i][j].getTag()!=table[i+2][j].getTag())
			{
					if(rand()%2==0)
						if(rand()%2==0)
							table[i+1][j]=table[i][j];
						else
							table[i+1][j]=table[i+2][j];
					i+=2;
			}
		}
	}
	//increase the chance of possible moves in horizental direction
	for(int i=0; i<NumberOfRows; i++)
	{	
		for(int j=0; j<NumberOfColumns-2; j++)
		{	
			if(table[i][j].getTag()!=table[i][j+1].getTag() && table[i][j+1].getTag()!=table[i][j+2].getTag() && table[i][j].getTag()!=table[i][j+2].getTag())
			{
				
					if(rand()%2==0)
					if(rand()%2==0)
						table[i][j+1]=table[i][j];
					else
						table[i][j+1]=table[i][j+2];
					j+=2;
			}
		}
	}
	// check for removing all match 3 states in vertical direction
	for(int j=0; j<NumberOfColumns; j++)
		for(int i=0; i<NumberOfRows-2; i++)	
		{
			if(table[i][j].getTag()==table[i+1][j].getTag() && table[i][j].getTag()==table[i+2][j].getTag())
				do
				{
					table[i][j].setTag((rand()%MaxDomain)+1);

					 c.i=i; c.j=j; 

					 numberOfMatchedElements=returnMatchCells(c,matchingsquare);
				} while(numberOfMatchedElements!=0);
			
		}
	// check for removing all match 3 states in horizental direction
	for(int i=0; i<NumberOfRows; i++)
		for(int j=0; j<NumberOfColumns-2; j++)
		{
			if(table[i][j].getTag()==table[i][j+1].getTag() && table[i][j].getTag()==table[i][j+2].getTag())
				do
				{
					table[i][j].setTag((rand()%MaxDomain)+1);
					c.i=i; c.j=j; 
					numberOfMatchedElements= returnMatchCells(c,matchingsquare);
				} while(numberOfMatchedElements!=0);
			
		}
}
int Matrix::returnMatchCells(coordination c, coordination* erasedCells)
{
	int val=table[c.i][c.j].getTag(),counter=0, start=-1, end=-1,i,j,numberOfElements=0;
	
	//find cells that must be erased in vertical direction
	for(i=c.i-2; i<c.i+3 && i<NumberOfRows; i++)
	{
		if(i<0 || i>NumberOfRows-1)
			continue;
		if(table[i][c.j].getTag()==val)
		{
			if(counter++==0)
				start=i;
		}
		else
		if(counter<3)
		{	
			counter=0;
			start=-1;
			if(i>c.i)
				break;
		}
		else
			break;
	}
	if(counter>2)
		end=i;
	for( i=start; i<end; i++)
	{
		erasedCells[i-start].i=i;
		erasedCells[i-start].j=c.j;
	}
	//find cells that must be erased in horizental direction
	int base=i-start;
	counter=0;
	start=-1, end=-1;
	for(j=c.j-2; j<c.j+3 && j<NumberOfColumns; j++)
		{
			if(j<0 || j>NumberOfColumns-1)
				continue;
			if(table[c.i][j].getTag()==val)
			{
				if(counter++==0)
					start=j;
			}
			else
			if(counter<3)
			{	
				counter=0;
				start=-1;
				if(j>c.j)
					break;
			}
			else
				break;
	}
	// this variable is used to avoid adding the coordination c for two time
	int jump=0;
	if(counter>2)
				end=j;
	for( j=start; j<end; j++)
	{
		if(j==c.j && base!=0 )
		{
			jump=1;
			continue;
		}
		erasedCells[base+j-start-jump].i=c.i;
		erasedCells[base+j-start-jump].j=j;
	}
	if(j>start)
		if(base!=0)
			numberOfElements=base+j-start-1;
		else
			numberOfElements=base+j-start;
	else
		numberOfElements=base;
	return numberOfElements;

}
	
bool Matrix::animateExchange(point p)
{
	coordination temp=Area.findTheSquarePlace(p);
	
	bool retVal=false;
	// if the source has been selected before and also the new click is in the area
	if(source.i!=-1 && temp.i!=-1)
	{
		
			destination=temp;
			table[source.i][source.j].getSelectAnimation()->setStop(true);
			retVal=true;
	}
	else
	//if the click is in the area and source and destination have not been specified
	if(temp.i!=-1)
	{
		source=temp;
		table[source.i][source.j].setSelectAnimation(table[source.i][source.j].getMetaData()->scale());
	}
	else
		//if the click is outside the area and the source has been determined before
	if(temp.i==-1 && source.i!=-1)
	{
		table[source.i][source.j].getSelectAnimation()->setStop(true);
		resetSourceDestination();
	}
	
	return retVal;
}
bool Matrix::checkNeighbours()
{
	if(source.i==destination.i)
		if(source.j-1==destination.j ||source.j+1== destination.j)
			return true;
	if(source.j==destination.j)
		if(source.i-1==destination.i ||source.i+1== destination.i)
			return true;
	return false;
}
void Matrix::resetSourceDestination()
{
	source.i=source.j=-1;
	destination=source;
}

void Matrix::move()
{
	
	table[source.i][source.j].getMetaData()->setEnd(table[destination.i][destination.j].getMetaData()->getStart());
	table[destination.i][destination.j].getMetaData()->setEnd(table[source.i][source.j].getMetaData()->getStart());

	table[source.i][source.j].getMetaData()->move(exchangeDuration);
	table[destination.i][destination.j].getMetaData()->move(exchangeDuration);
	localTimer->setLength(exchangeDuration);
	localTimer->Resume();
	//

}
void Matrix::animateInitialization(Timer* timer)
{
	//set timer length between initiateAnimation(is defined in resource.h) +-0.1f
	srand((unsigned)time(NULL));
	float le=(float)rand();
	le=le/(5*RAND_MAX)+initiateAnimationDuration-0.1f;
	timer->setLength(le);


	Matrix* m=this;
	int columnCounter=m->rTA->getRandomColumn();
	if(columnCounter==-1)
	{
		if(!m->rTA->nextRow())
			{	
				timer->setLength(initiateMovementDuration,0);
				timer->setTimerState(timer->transition);
				return;
			}
		columnCounter=m->rTA->getRandomColumn();
	}
	m->animateFirstCoordination(columnCounter);

}
void Matrix::eraseAfterDelay()
{
	square* sTemp;
	//exchange the squares
	if(source.i!=-1)
	{
		sTemp=table[source.i][source.j].getMetaData();
		table[source.i][source.j].setMetaData(table[destination.i][destination.j].getMetaData());
		table[destination.i][destination.j].setMetaData(sTemp);
	}
	
	
	erase();
	int maxSquareDeleted;
	for(int j=0; j<NumberOfColumns; j++)
	{
		maxSquareDeleted=0;
		for(int i=NumberOfRows-1;i>-1;i--)
			if(table[i][j].getIsDeleted())
				maxSquareDeleted++;
			else
				table[i][j].getMetaData()->setDownwardMovement(maxSquareDeleted);
		eraseManager::specifymaxDownwardMovInCol(j, maxSquareDeleted);
	}

	localTimer->setLength(eraseDuration,0);
	localTimer->setTimerState(localTimer->ReadyToFill);
	
}
void Matrix::exchangeTagsBetweenSourceAndDes()
{
	//exchange the tags
	int temp=table[source.i][source.j].getTag();
	table[source.i][source.j].setTag(table[destination.i][destination.j].getTag());
	table[destination.i][destination.j].setTag(temp);
}
void Matrix::playErrorSound()
{
		char **s=new (char*)();
		table[source.i][source.j].mapValuesToAnimals(s);
		g_pAudio->PlaySound(*s);
		delete s;
}
void Matrix::erase()
{
	coordination c;
	int num=matchingSquareManager::getTop();
	//erase matching squares in an animated fashion
	for(int k=0; k<=num; k++)
	{
		c=matchingSquareManager::pickOne();
		table[c.i][c.j].setDeleted(true);
		table[c.i][c.j].getMetaData()->erase();
		//eraseManager::addToDeletedSquare(c.j,table[c.i][c.j].getMetaData());
	}
}
void Matrix::rollback()
{
	exchangeTagsBetweenSourceAndDes();
	playErrorSound();
	move();
	resetSourceDestination();
	parent->SetInputActive(true);
	localTimer->setTimerState(localTimer->readyToExchange);
	localTimer->Pause();
}
void Matrix::fillEmptySquare()
{
	
	if(eraseManager::getMaxDownWardMov()>0)
	{
		eraseManager::MaxDownWardMovMinusMinus();
		for( int j=0;j<NumberOfColumns;j++)
			for(int i=NumberOfRows-1; i>-1;i--)
			{
			point p={table[i][j].getMetaData()->getData()->m_X, table[i][j].getMetaData()->getData()->m_Y+Area.getSquareLength()};
			table[i][j].getMetaData()->moveDown();
			//make invisibe squares that are entered in the table area
			if(table[i][j].getMetaData()->getData()->m_IsVisible==false)
				if(Area.checkPointHitTheArea(p))
					table[i][j].getMetaData()->getData()->m_IsVisible=true;
			}
	}
	else
	{
		eraseManager::reset();
		matchingSquareManager::clear();
		refreshTable();
		resetSourceDestination();


		coordination cwww[81];
		int num=returnAllMatchedSquares(cwww);
		if(num!=0)
		{
			matchingSquareManager::add(cwww,num);
			eraseAfterDelay();
			return;
		}
		
		parent->SetInputActive(true);

		localTimer->setTimerState(localTimer->readyToExchange);
		localTimer->Pause();
	}
	
	
			
}
void Matrix::ReadyToFill()
{
	int j,i,k,counter=0,localTag,constDownWard;
	srand((unsigned)time(NULL));
	
	for( j=0;j<NumberOfColumns;j++)
	{
		k=NumberOfRows-1;
		for(i=NumberOfRows-1; i>-1;i--)
		{
				if(table[i][j].getIsDeleted())
				{
						//keep the address of the squares that have already been deleted in order to reuse them as a new square
						eraseManager::addToDeletedSquare(j,table[i][j].getMetaData());
				}
				else
				{
					if(k-i>0)
					{
						//update tag
						table[k][j].setTag(table[i][j].getTag());
						//set the new square to update square places
						table[k][j].setMetaData(table[i][j].getMetaData());
						//set downward movement for squares
						table[k][j].getMetaData()->setDownwardMovement(k-i);
					}
					k--;
				}
		}
		constDownWard=k-i;
		counter=0;
		//reuse deleted square
		for(i=k;k>-1;k--)
		{
				localTag=rand()%MaxDomain+1;
				table[k][j].setTag(localTag);
				square* te=eraseManager::getItemFromDeletedSquare(counter,j);
				//set the deleted square, invisible
				
				table[k][j].setMetaData(te);
				te->prepareSquareToReuse(table[0][j].getAnchor(),table[k][j].mapValuesToAnimals(),counter+1);
				te->getData()->m_IsVisible=false;
				table[k][j].getMetaData()->setDownwardMovement(constDownWard);
				counter++;
		}
	}
}

void Matrix::refreshTable()
{
	for(int i=0;i<NumberOfRows;i++)
		for(int j=0; j<NumberOfColumns;j++)
			table[i][j].setDeleted(false);
}
int Matrix::returnAllMatchedSquares(coordination* c )
{
	int val,counter=1,j,i, index=0;
	
	for(i=0;i<NumberOfRows;i++)
	{
		for(int j=0;j<NumberOfColumns-2; j++)
		{
			counter=1;
			val=table[i][j].getTag();
			for(int k=j+1; k<NumberOfColumns;k++)
			{
				
				if(table[i][k].getTag()==val)
					counter++;
				if(table[i][k].getTag()!=val || k==NumberOfColumns-1)
				{	
					if(counter>=3)
						for(int t=j; t<k;t++)
						{
								if(table[i][t].getIsDeleted())
									continue;
								c[index].i=i;
								c[index++].j=t;
						}
					j=k-1;
					break;
				}
			}
		}
	}
	
		for(int j=0;j<NumberOfColumns; j++)
		{
			for(i=0;i<NumberOfRows-2;i++)
			{
			counter=1;
			val=table[i][j].getTag();
			for(int k=i+1; k<NumberOfRows;k++)
			{
				
				if(table[k][j].getTag()==val)
					counter++;
				if(table[k][j].getTag()!=val || k==NumberOfRows-1)
				{	
					if(counter>=3)
						for(int t=i; t<k;t++)
						{
								if(table[t][j].getIsDeleted())
									continue;
								c[index].i=t;
								c[index++].j=j;
						}
					i=k-1;
					break;
				}
			}
		}
	}
return index;

}
