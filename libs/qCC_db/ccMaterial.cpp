//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

//Always first
#include "ccIncludeGL.h"

//Local
#include "ccMaterial.h"
#include "ccBasicTypes.h"
#include "ccLog.h"

//Qt
#include <QMap>
#include <QUuid>
#include <QFileInfo>
#include <QDataStream>

//System
#include <string.h>
#include <assert.h>

//Textures DB
QMap<QString, QImage> s_textureDB;

ccMaterial::ccMaterial(QString name)
	: m_name(name)
	, m_uniqueID(QUuid::createUuid().toString())
{
	memcpy(m_diffuseFront, ccColor::bright, sizeof(float)*4);
	memcpy(m_diffuseBack,  ccColor::bright, sizeof(float)*4);
	memcpy(m_ambient,      ccColor::night,  sizeof(float)*4);
	memcpy(m_specular,     ccColor::night,  sizeof(float)*4);
	memcpy(m_emission,     ccColor::night,  sizeof(float)*4);
	setShininess(50.0);
};

ccMaterial::ccMaterial(const ccMaterial& mtl)
	: m_name(mtl.m_name)
	, m_textureFilename(mtl.m_textureFilename)
	, m_shininessFront(mtl.m_shininessFront)
	, m_shininessBack(mtl.m_shininessFront)
	, m_uniqueID(mtl.m_uniqueID)
{
	memcpy(m_diffuseFront, mtl.m_diffuseFront, sizeof(float)*4);
	memcpy(m_diffuseBack,  mtl.m_diffuseBack,  sizeof(float)*4);
	memcpy(m_ambient,      mtl.m_ambient,      sizeof(float)*4);
	memcpy(m_specular,     mtl.m_specular,     sizeof(float)*4);
	memcpy(m_emission,     mtl.m_emission,     sizeof(float)*4);
}

void ccMaterial::setDiffuse(const float color[4])
{
	setDiffuseFront(color);
	setDiffuseBack(color);
}

void ccMaterial::setDiffuseFront(const float color[4])
{
	memcpy(m_diffuseFront, color, sizeof(float)*4);
}

void ccMaterial::setDiffuseBack(const float color[4])
{
	memcpy(m_diffuseBack, color, sizeof(float)*4);
}

void ccMaterial::setAmbient(const float color[4])
{
	memcpy(m_ambient, color, sizeof(float)*4);
}

void ccMaterial::setSpecular(const float color[4])
{
	memcpy(m_specular, color, sizeof(float)*4);
}

void ccMaterial::setEmission(const float color[4])
{
	memcpy(m_emission, color, sizeof(float)*4);
}

void ccMaterial::setShininess(float val)
{
	setShininessFront(val);
	setShininessBack(0.8f * val);
}

void ccMaterial::setShininessFront(float val)
{
	m_shininessFront = val;
}

void ccMaterial::setShininessBack(float val)
{
	m_shininessBack = val;
}

void ccMaterial::setTransparency(float val)
{
	m_diffuseFront[3] = val;
	m_diffuseBack[3]  = val;
	m_ambient[3]      = val;
	m_specular[3]     = val;
	m_emission[3]     = val;
}

void ccMaterial::applyGL(bool lightEnabled, bool skipDiffuse) const
{
	if (lightEnabled)
	{
		if (!skipDiffuse)
		{
			glMaterialfv(GL_FRONT, GL_DIFFUSE, m_diffuseFront);
			glMaterialfv(GL_BACK,  GL_DIFFUSE, m_diffuseBack);
		}
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   m_ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  m_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  m_emission);
		glMaterialf (GL_FRONT,          GL_SHININESS, m_shininessFront);
		glMaterialf (GL_BACK,           GL_SHININESS, m_shininessBack);
	}
	else
	{
		glColor4fv(m_diffuseFront);
	}
}

bool ccMaterial::loadAndSetTexture(QString absoluteFilename)
{
	if (absoluteFilename.isEmpty())
	{
		ccLog::Warning(QString("[ccMaterial::loadAndSetTexture] filename can't be empty!"));
		return false;
	}
	ccLog::PrintDebug(QString("[ccMaterial::loadAndSetTexture] absolute filename = %1").arg(absoluteFilename));

	if (s_textureDB.contains(absoluteFilename))
	{
		//if the image is already in memory, we simply update the texture filename for this amterial
		m_textureFilename = absoluteFilename;
	}
	else
	{
		//otherwise, we try to load the corresponding file
		QImage image(absoluteFilename);
		if (image.isNull())
		{
			ccLog::Warning(QString("[ccMaterial::loadAndSetTexture] Failed to load image '%1'").arg(absoluteFilename));
			return false;
		}
		else
		{
			setTexture(image,absoluteFilename,true);
		}
	}

	return true;
}

void ccMaterial::setTexture(QImage image, QString absoluteFilename/*=QString()*/, bool mirrorImage/*=true*/)
{
	ccLog::PrintDebug(QString("[ccMaterial::setTexture] absoluteFilename = %1 (+ image(%2,%3)").arg(absoluteFilename).arg(image.width()).arg(image.height()));

	if (absoluteFilename.isEmpty())
	{
		//if the user hasn't provided any filename, we generate a fake one
		absoluteFilename = QString("tex_%1.jpg").arg(m_uniqueID);
		assert(!s_textureDB.contains(absoluteFilename));
	}
	else
	{
		//if the texture has already been loaded
		if (s_textureDB.contains(absoluteFilename))
		{
			//check that the size is compatible at least
			if (s_textureDB[absoluteFilename].size() != image.size())
			{
				ccLog::Warning(QString("[ccMaterial] A texture with the same name (%1) but with a different size has already been loaded!").arg(absoluteFilename));
			}
			m_textureFilename = absoluteFilename;
			return;
		}
	}

	m_textureFilename = absoluteFilename;

	//insert image into DB if necessary
	s_textureDB[m_textureFilename] = mirrorImage ? image.mirrored() : image;
}

const QImage ccMaterial::getTexture() const
{
	return s_textureDB[m_textureFilename];
}

bool ccMaterial::hasTexture() const
{
	if (m_textureFilename.isEmpty())
		return false;

	return !s_textureDB[m_textureFilename].isNull();
}

void ccMaterial::MakeLightsNeutral()
{
	GLint maxLightCount;
	glGetIntegerv(GL_MAX_LIGHTS,&maxLightCount);
	
	for (int i=0; i<maxLightCount; ++i)
	{
		if (glIsEnabled(GL_LIGHT0+i))
		{
			float diffuse[4];
			float ambiant[4];
			float specular[4];

			glGetLightfv(GL_LIGHT0+i,GL_DIFFUSE,diffuse);
			glGetLightfv(GL_LIGHT0+i,GL_AMBIENT,ambiant);
			glGetLightfv(GL_LIGHT0+i,GL_SPECULAR,specular);

			 diffuse[0] =  diffuse[1] =  diffuse[2] = ( diffuse[0] +  diffuse[1] +  diffuse[2]) / 3.0f;	//'mean' (gray) value
			 ambiant[0] =  ambiant[1] =  ambiant[2] = ( ambiant[0] +  ambiant[1] +  ambiant[2]) / 3.0f;	//'mean' (gray) value
			specular[0] = specular[1] = specular[2] = (specular[0] + specular[1] + specular[2]) / 3.0f;	//'mean' (gray) value

			glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);
			glLightfv(GL_LIGHT0+i, GL_AMBIENT, ambiant);
			glLightfv(GL_LIGHT0+i,GL_SPECULAR,specular);
		}
	}
}

QImage ccMaterial::GetTexture(QString absoluteFilename)
{
	return s_textureDB[absoluteFilename];
}

void ccMaterial::AddTexture(QImage image, QString absoluteFilename)
{
	s_textureDB[absoluteFilename] = image;
}

bool ccMaterial::toFile(QFile& out) const
{
	QDataStream outStream(&out);

	//material name (dataVersion>=20)
	outStream << m_name;
	//texture (dataVersion>=20)
	outStream << m_textureFilename;
	//material colors (dataVersion>=20)
	//we don't use QByteArray here as it has its own versions!
	if (out.write((const char*)m_diffuseFront,sizeof(float)*4) < 0) 
		return WriteError();
	if (out.write((const char*)m_diffuseBack,sizeof(float)*4) < 0) 
		return WriteError();
	if (out.write((const char*)m_ambient,sizeof(float)*4) < 0) 
		return WriteError();
	if (out.write((const char*)m_specular,sizeof(float)*4) < 0) 
		return WriteError();
	if (out.write((const char*)m_emission,sizeof(float)*4) < 0) 
		return WriteError();
	//material shininess (dataVersion>=20)
	outStream << m_shininessFront;
	outStream << m_shininessBack;

	return true;
}

bool ccMaterial::fromFile(QFile& in, short dataVersion, int flags)
{
	QDataStream inStream(&in);

	//material name (dataVersion>=20)
	inStream >> m_name;
	if (dataVersion < 37)
	{
		//texture (dataVersion>=20)
		QImage texture;
		inStream >> texture;
		setTexture(texture,QString(),false);
	}
	else
	{
		//texture 'filename' (dataVersion>=37)
		inStream >> m_textureFilename;
	}
	//material colors (dataVersion>=20)
	if (in.read((char*)m_diffuseFront,sizeof(float)*4) < 0) 
		return ReadError();
	if (in.read((char*)m_diffuseBack,sizeof(float)*4) < 0) 
		return ReadError();
	if (in.read((char*)m_ambient,sizeof(float)*4) < 0) 
		return ReadError();
	if (in.read((char*)m_specular,sizeof(float)*4) < 0) 
		return ReadError();
	if (in.read((char*)m_emission,sizeof(float)*4) < 0) 
		return ReadError();
	//material shininess (dataVersion>=20)
	inStream >> m_shininessFront;
	inStream >> m_shininessBack;

	return true;
}
